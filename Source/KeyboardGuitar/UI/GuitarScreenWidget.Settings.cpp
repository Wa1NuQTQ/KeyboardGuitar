#include "GuitarScreenWidget.h"
#include "GuitarTabStyle.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Practice/GuitarPracticeSessionComponent.h"

void UGuitarScreenWidget::BindSettingsControls()
{
	auto Bind = [this](UUserWidget* Page, const TCHAR* Name, FName Handler)
	{
		UButton* Button = Cast<UButton>(Page->GetWidgetFromName(Name));
		if (!Button) return;
		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, Handler);
		Button->OnClicked.AddUnique(Delegate);
		BoundButtons.Add(Button);
	};
	for (EPage PageId : {EPage::SongSelect, EPage::Pause, EPage::Results})
		Bind(GetPage(PageId), TEXT("SettingsButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, OpenSettings));
	for (EPage PageId : {EPage::SongSelect, EPage::Pause, EPage::Results, EPage::Settings})
		Bind(GetPage(PageId), TEXT("QuitButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, Quit));
	Bind(GetPage(EPage::Settings), TEXT("CloseSettingsButton"), GET_FUNCTION_NAME_CHECKED(UGuitarScreenWidget, CloseSettings));
	MusicVolumeSlider = CastChecked<USlider>(GetPage(EPage::Settings)->GetWidgetFromName(TEXT("MusicVolumeSlider")));
	MusicVolumeSlider->OnValueChanged.AddDynamic(this, &UGuitarScreenWidget::ChangeMusicVolume);
	SetMusicVolume(MusicVolume);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		UTextBlock* Label = CastChecked<UTextBlock>(GetPage(EPage::Settings)->GetWidgetFromName(*FString::Printf(TEXT("ChordType%dText"), Index)));
		Label->SetColorAndOpacity(GuitarUI::GetTabColors(static_cast<EGuitarChordType>(Index)).Ink);
	}
}

void UGuitarScreenWidget::OpenSettings()
{
	ShowSettings();
}

void UGuitarScreenWidget::ShowSettings(bool bFromMovement)
{
	// 先让会话暂停并释放持键，再切换设置页；关闭设置仍停留在暂停页。
	if (!bFromMovement && Session && (Session->CanPlay() || Session->GetStage() == EGuitarPracticeStage::Countdown)) Pause();
	FinishSongTransition();
	SettingsReturnPage = bFromMovement ? EPage::None : static_cast<EPage>(ScreenSwitcher->GetActiveWidgetIndex());
	bExitOnTabRelease = false;
	ScreenSwitcher->SetActiveWidget(GetPage(EPage::Settings));
	SetFocus();
}

void UGuitarScreenWidget::CloseSettings()
{
	ScreenSwitcher->SetActiveWidget(GetPage(SettingsReturnPage == EPage::None ? EPage::SongSelect : SettingsReturnPage));
	SetFocus();
	OnSettingsClosed.Broadcast();
}

void UGuitarScreenWidget::Quit()
{
	OnQuitRequested.Broadcast();
}

void UGuitarScreenWidget::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (MusicVolumeSlider) MusicVolumeSlider->SetValue(MusicVolume);
	SetText(GetPage(EPage::Settings), TEXT("MusicVolumeText"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(MusicVolume * 100)));
}

void UGuitarScreenWidget::ChangeMusicVolume(float Volume)
{
	SetMusicVolume(Volume);
	OnMusicVolumeChanged.Broadcast(MusicVolume);
}
