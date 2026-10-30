#include "GuitarScreenWidget.h"
#include "GuitarChordLabel.h"
#include "GuitarTabStyle.h"
#include "GuitarTabWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/Pawn.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Practice/GuitarSong.h"

void UGuitarScreenWidget::SetText(UUserWidget* Page, FName Name, const FString& Value)
{
	if (UTextBlock* Text = Page ? Cast<UTextBlock>(Page->GetWidgetFromName(Name)) : nullptr)
	{
		// 未变化的文字不触发布局重建
		if (Text->GetText().ToString() != Value) Text->SetText(FText::FromString(Value));
	}
}

void UGuitarScreenWidget::RefreshSongCards()
{
	if (Songs.IsEmpty()) return;
	SelectedSongIndex = FMath::Clamp(SelectedSongIndex, 0, Songs.Num() - 1);
	const UGuitarSong* Song = GetSelectedSong();
	UUserWidget* Page = GetPage(EPage::SongSelect);
	SetText(Page, TEXT("SongTitleText"), Song->Title.ToString());
	SetText(Page, TEXT("PreviousSongTitleText"), Songs[(SelectedSongIndex + Songs.Num() - 1) % Songs.Num()]->Title.ToString());
	SetText(Page, TEXT("NextSongTitleText"), Songs[(SelectedSongIndex + 1) % Songs.Num()]->Title.ToString());
	const int32 Seconds = FMath::RoundToInt(Song->DurationSeconds);
	const int32 Best = Session ? Session->GetBestScore(Song->GetFName()) : 0;
	SetText(Page, TEXT("SongMetadataText"), FString::Printf(TEXT("%d BPM  ·  %02d:%02d\nBEST %d"), Song->BeatsPerMinute, Seconds / 60, Seconds % 60, Best));
}

void UGuitarScreenWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	UpdateSongTransition();
	if (!Session) return;
	const EPage Page = static_cast<EPage>(ScreenSwitcher->GetActiveWidgetIndex());
	if (Page == EPage::Countdown)
	{
		SetText(GetPage(EPage::Countdown), TEXT("CountdownText"), FString::FromInt(FMath::Max(1, FMath::CeilToInt(-Session->GetSongTimeSeconds()))));
		RefreshPerformance(Cast<UUserWidget>(GetPage(EPage::Countdown)->GetWidgetFromName(TEXT("PerformancePreview"))));
	}
	else if (Page == EPage::Performance) RefreshPerformance(GetPage(EPage::Performance));
	else if (Page == EPage::Pause) RefreshPerformance(Cast<UUserWidget>(GetPage(EPage::Pause)->GetWidgetFromName(TEXT("PerformancePreview"))));
}

void UGuitarScreenWidget::RefreshPerformance(UUserWidget* Page)
{
	if (!Page) return;
	const UGuitarSong* Song = Session->GetSong();
	const double Time = Session->GetSongTimeSeconds();
	SetText(Page, TEXT("SongTitleText"), Song ? Song->Title.ToString() : TEXT("自由演奏"));
	const FGuitarPracticeResult& Result = Session->GetResult();
	SetText(Page, TEXT("ScoreText"), Song ? FString::FromInt(Result.Score) : TEXT(""));
	SetText(Page, TEXT("ComboText"), Song ? FString::FromInt(Result.Combo) : TEXT(""));
	SetText(Page, TEXT("ComboLabelText"), Song ? TEXT("连击") : TEXT(""));
	APawn* Pawn = GetOwningPlayerPawn();
	const UGuitarInstrumentComponent* Instrument = Pawn ? Pawn->FindComponentByClass<UGuitarInstrumentComponent>() : nullptr;
	const FName Actual = Instrument ? Instrument->GetCurrentChordId() : NAME_None;
	for (UGuitarTabWidget* Tab : Tabs) Tab->SetPlayedChord(Actual);
	// 跟曲只显示滚动目标；自由演奏没有谱面目标，保留实际和弦状态。
	UTextBlock* HeldChord = CastChecked<UTextBlock>(Page->GetWidgetFromName(TEXT("HeldChordText")));
	HeldChord->SetVisibility(Song ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	SetText(Page, TEXT("HeldChordText"), GuitarUI::MakeChordLabel(Actual));
	HeldChord->SetColorAndOpacity(FSlateColor(GuitarUI::GetChordColor(Actual)));
	SetText(Page, TEXT("MetronomeButtonText"), bMetronomeEnabled ? TEXT("节拍 开") : TEXT("节拍 关"));
	FString Feedback;
	if (Session->GetFeedbackAgeSeconds() < 0.7)
	{
		switch (Session->GetLastJudgement())
		{
		case EGuitarJudgement::Perfect: Feedback = TEXT("Perfect"); break;
		case EGuitarJudgement::Good: Feedback = TEXT("Good"); break;
		case EGuitarJudgement::Wrong: Feedback = TEXT("Wrong"); break;
		case EGuitarJudgement::Miss: Feedback = TEXT("Miss"); break;
		default: break;
		}
	}
	SetText(Page, TEXT("JudgmentText"), Feedback);
	if (UWidget* Progress = Page->GetWidgetFromName(TEXT("SongProgressImage")))
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Progress->Slot))
		{
			const float Fraction = Song ? FMath::Clamp(Time / Song->DurationSeconds, 0.0, 1.0) : 0.0;
			// 进度条沿用底轨宽度，调整 HUD 布局时无需同步修改数值。
			const UWidget* Track = Page->GetWidgetFromName(TEXT("ProgressTrackImage"));
			const UCanvasPanelSlot* TrackSlot = CastChecked<UCanvasPanelSlot>(Track->Slot);
			CanvasSlot->SetSize(FVector2D(TrackSlot->GetSize().X * Fraction, CanvasSlot->GetSize().Y));
		}
	}
}

void UGuitarScreenWidget::SetMetronomeEnabled(bool bEnabled)
{
	bMetronomeEnabled = bEnabled;
}

void UGuitarScreenWidget::SetChordType(EGuitarChordType Type)
{
	SelectedChordType = Type;
	const FGuitarTabColors& Colors = GuitarUI::GetTabColors(Type);
	for (UGuitarTabWidget* Tab : Tabs) Tab->SetChordType(Type);
	// 倒数、演奏与暂停共用选择状态；松开根音或暂停不重置类型颜色。
	for (EPage PageId : {EPage::Countdown, EPage::Performance, EPage::Pause})
	{
		UUserWidget* HUD = PageId == EPage::Performance ? GetPage(EPage::Performance) : CastChecked<UUserWidget>(GetPage(PageId)->GetWidgetFromName(TEXT("PerformancePreview")));
		UImage* Background = CastChecked<UImage>(HUD->GetWidgetFromName(TEXT("ChartBackgroundImage")));
		Background->SetBrushTintColor(FSlateColor(FLinearColor::White));
		Background->SetColorAndOpacity(Colors.Paper);
	}
}

void UGuitarScreenWidget::RefreshResults()
{
	const UGuitarSong* Song = Session->GetSong();
	if (!Song) return;
	const FGuitarPracticeResult& Result = Session->GetResult();
	const float Accuracy = 100.0f * Result.Score / (Song->Plucks.Num() * 1000.0f);
	SetText(GetPage(EPage::Results), TEXT("SongTitleText"), Song->Title.ToString());
	SetText(GetPage(EPage::Results), TEXT("FinalScoreText"), FString::FromInt(Result.Score));
	SetText(GetPage(EPage::Results), TEXT("BestComboText"), FString::Printf(TEXT("%.1f%%  ·  %d 连击\nP %d / G %d / W %d / M %d"), Accuracy, Result.BestCombo, Result.Perfect, Result.Good, Result.Wrong, Result.Miss));
	SetText(GetPage(EPage::Results), TEXT("SaveStatusText"), Session->WasRecordSaved() ? TEXT("") : TEXT("成绩保存失败"));
}
