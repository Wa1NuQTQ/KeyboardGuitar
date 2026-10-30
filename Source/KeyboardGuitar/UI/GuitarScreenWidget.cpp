#include "GuitarScreenWidget.h"
#include "GuitarTabWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Slider.h"
#include "InputCoreTypes.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Practice/GuitarSong.h"

void UGuitarScreenWidget::Configure(UGuitarPracticeSessionComponent* InSession, const TArray<TObjectPtr<UGuitarSong>>& InSongs)
{
	Session = InSession;
	Songs = InSongs;
	Tabs.Reset();
	// 倒数、演奏、暂停各有一份 HUD，替换运行时的静态谱面样例
	for (EPage PageId : {EPage::Countdown, EPage::Performance, EPage::Pause})
	{
		UUserWidget* Page = GetPage(PageId);
		UUserWidget* HUD = PageId == EPage::Performance ? Page : Cast<UUserWidget>(Page->GetWidgetFromName(TEXT("PerformancePreview")));
		UCanvasPanel* Timeline = HUD ? Cast<UCanvasPanel>(HUD->GetWidgetFromName(TEXT("TimelineCanvas"))) : nullptr;
		if (!Timeline) continue;
		Timeline->ClearChildren();
		UGuitarTabWidget* Tab = WidgetTree->ConstructWidget<UGuitarTabWidget>();
		Tab->Configure(Session);
		Tab->SetChordType(SelectedChordType);
		Tabs.Add(Tab);
		UCanvasPanelSlot* CanvasSlot = Timeline->AddChildToCanvas(Tab);
		CanvasSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		CanvasSlot->SetOffsets(FMargin(0));
	}
}

UUserWidget* UGuitarScreenWidget::GetPage(EPage Page) const
{
	return Cast<UUserWidget>(ScreenSwitcher->GetWidgetAtIndex(static_cast<int32>(Page)));
}

void UGuitarScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	auto Bind = [this](UUserWidget* Page, const TCHAR* Name, FName Handler)
	{
		UButton* Button = Page ? Cast<UButton>(Page->GetWidgetFromName(Name)) : nullptr;
		if (!Button) return;
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, Handler);
		Button->OnClicked.AddUnique(Delegate);
		BoundButtons.Add(Button);
	};
	Bind(GetPage(EPage::SongSelect), TEXT("BackButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, RequestExit));
	Bind(GetPage(EPage::SongSelect), TEXT("StartButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, Start));
	Bind(GetPage(EPage::SongSelect), TEXT("FreePlayButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, FreePlay));
	Bind(GetPage(EPage::SongSelect), TEXT("PreviousSongButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, PreviousSong));
	Bind(GetPage(EPage::SongSelect), TEXT("NextSongButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, NextSong));
	Bind(GetPage(EPage::Pause), TEXT("ResumeButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, Resume));
	Bind(GetPage(EPage::Pause), TEXT("RestartButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, Restart));
	Bind(GetPage(EPage::Pause), TEXT("SongSelectButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, SelectSongs));
	Bind(GetPage(EPage::Results), TEXT("RetryButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, Restart));
	Bind(GetPage(EPage::Results), TEXT("SongSelectButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, SelectSongs));
	for (EPage PageId : {EPage::Countdown, EPage::Performance, EPage::Pause})
	{
		UUserWidget* HUD = PageId == EPage::Performance ? GetPage(EPage::Performance) : Cast<UUserWidget>(GetPage(PageId)->GetWidgetFromName(TEXT("PerformancePreview")));
		Bind(HUD, TEXT("MetronomeButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, ToggleMetronome));
	}
	BindSettingsControls();
	CacheSongCards();
	RefreshSongCards();
}

void UGuitarScreenWidget::NativeDestruct()
{
	bExitOnTabRelease = false;
	FinishSongTransition();
	for (UButton* Button : BoundButtons) Button->OnClicked.RemoveAll(this);
	BoundButtons.Reset();
	if (MusicVolumeSlider) MusicVolumeSlider->OnValueChanged.RemoveAll(this);
	MusicVolumeSlider = nullptr;
	Super::NativeDestruct();
}

UGuitarSong* UGuitarScreenWidget::GetSelectedSong() const
{
	return Songs.IsValidIndex(SelectedSongIndex) ? Songs[SelectedSongIndex] : nullptr;
}

void UGuitarScreenWidget::ShowSongSelection()
{
	FinishSongTransition();
	ScreenSwitcher->SetActiveWidget(GetPage(EPage::SongSelect));
	RefreshSongCards();
}

void UGuitarScreenWidget::ShowSessionPage()
{
	if (!Session) return;
	switch (Session->GetStage())
	{
	case EGuitarPracticeStage::Selecting: ShowSongSelection(); break;
	case EGuitarPracticeStage::Countdown: ScreenSwitcher->SetActiveWidget(GetPage(EPage::Countdown)); break;
	case EGuitarPracticeStage::Playing:
	case EGuitarPracticeStage::FreePlay: ScreenSwitcher->SetActiveWidget(GetPage(EPage::Performance)); break;
	case EGuitarPracticeStage::Paused: ScreenSwitcher->SetActiveWidget(GetPage(EPage::Pause)); break;
	case EGuitarPracticeStage::Results: ScreenSwitcher->SetActiveWidget(GetPage(EPage::Results)); RefreshResults(); break;
	}
}

void UGuitarScreenWidget::RequestExit() { OnExitRequested.Broadcast(); }
void UGuitarScreenWidget::Start() { FinishSongTransition(); OnStartRequested.Broadcast(); }
void UGuitarScreenWidget::FreePlay() { FinishSongTransition(); OnFreePlayRequested.Broadcast(); }
void UGuitarScreenWidget::Pause() { OnPauseRequested.Broadcast(); }
void UGuitarScreenWidget::Resume() { OnResumeRequested.Broadcast(); }
void UGuitarScreenWidget::Restart() { OnRestartRequested.Broadcast(); }
void UGuitarScreenWidget::SelectSongs() { OnSongSelectionRequested.Broadcast(); }
void UGuitarScreenWidget::ToggleMetronome() { OnMetronomeToggleRequested.Broadcast(); }

FReply UGuitarScreenWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (ScreenSwitcher->GetActiveWidget() == GetPage(EPage::Settings))
	{
		if (Key == EKeys::Escape)
		{
			if (!Event.IsRepeat()) CloseSettings();
			return FReply::Handled();
		}
		return Super::NativeOnPreviewKeyDown(Geometry, Event);
	}
	if (Key == EKeys::Tab)
	{
		if (!Event.IsRepeat()) bExitOnTabRelease = true;
		return FReply::Handled();
	}
	if (Event.IsRepeat()) return Super::NativeOnPreviewKeyDown(Geometry, Event);
	if (Key == EKeys::M)
	{
		ToggleMetronome();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape)
	{
		if (Session && Session->GetStage() == EGuitarPracticeStage::Paused) Resume();
		else if (Session && (Session->CanPlay() || Session->GetStage() == EGuitarPracticeStage::Countdown)) Pause();
		else if (Session && Session->GetStage() == EGuitarPracticeStage::Results) SelectSongs();
		else RequestExit();
		return FReply::Handled();
	}
	if (Session && Session->GetStage() == EGuitarPracticeStage::Selecting)
	{
		if (Key == EKeys::Left) PreviousSong();
		else if (Key == EKeys::Right) NextSong();
		else if (Key == EKeys::Enter) Start();
		else if (Key == EKeys::F) FreePlay();
		else return Super::NativeOnPreviewKeyDown(Geometry, Event);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

FReply UGuitarScreenWidget::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (ScreenSwitcher->GetActiveWidget() == GetPage(EPage::Settings)) return Super::NativeOnKeyUp(Geometry, Event);
	if (Event.GetKey() == EKeys::Tab)
	{
		const bool bExit = bExitOnTabRelease;
		bExitOnTabRelease = false;
		if (bExit)
		{
			if (Session && (Session->CanPlay() || Session->GetStage() == EGuitarPracticeStage::Countdown)) Pause();
			else RequestExit();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(Geometry, Event);
}
