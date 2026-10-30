#include "GuitarScreenWidget.h"
#include "Components/WidgetSwitcher.h"

namespace
{
	constexpr float TransitionDurationSeconds = 0.28f;
	constexpr float SlideDistance = 48.0f;
}

void UGuitarScreenWidget::CacheSongCards()
{
	if (!SongCards.IsEmpty()) return;
	for (const TCHAR* Name : {TEXT("PreviousSongPanel"), TEXT("SelectedSongPanel"), TEXT("NextSongPanel")})
	{
		UWidget* Card = GetPage(EPage::SongSelect)->GetWidgetFromName(Name);
		if (!Card) continue;
		SongCards.Add(Card);
		SongCardTranslations.Add(Card->GetRenderTransform().Translation);
		SongCardOpacities.Add(Card->GetRenderOpacity());
	}
}

void UGuitarScreenWidget::PreviousSong() { QueueSongChange(-1); }
void UGuitarScreenWidget::NextSong() { QueueSongChange(1); }

void UGuitarScreenWidget::QueueSongChange(int32 Direction)
{
	if (Songs.Num() < 2) return;
	if (bSongTransitionActive)
	{
		// 连续点击最多排两次，反向输入抵消待切换方向，避免积累很长的动画队列
		PendingSongSteps = FMath::Clamp(PendingSongSteps + Direction, -2, 2);
		return;
	}
	SongTransitionDirection = Direction;
	SongTransitionStartSeconds = FPlatformTime::Seconds();
	bSongTransitionCommitted = false;
	bSongTransitionActive = true;
}

void UGuitarScreenWidget::UpdateSongTransition()
{
	if (!bSongTransitionActive || ScreenSwitcher->GetActiveWidget() != GetPage(EPage::SongSelect)) return;
	// 使用真实时间，低帧率下也不拉长动画；队列接续保留本帧越过的时间
	const double Now = FPlatformTime::Seconds();
	while (bSongTransitionActive)
	{
		const float Alpha = FMath::Clamp(static_cast<float>((Now - SongTransitionStartSeconds) / TransitionDurationSeconds), 0.0f, 1.0f);
		const bool bIncoming = Alpha >= 0.5f;
		if (bIncoming && !bSongTransitionCommitted)
		{
			SelectedSongIndex = (SelectedSongIndex + SongTransitionDirection + Songs.Num()) % Songs.Num();
			bSongTransitionCommitted = true;
			RefreshSongCards();
		}
		const float HalfAlpha = bIncoming ? (Alpha - 0.5f) * 2.0f : Alpha * 2.0f;
		const float Ease = FMath::SmoothStep(0.0f, 1.0f, HalfAlpha);
		const float Opacity = bIncoming ? Ease : 1.0f - Ease;
		const float Translation = SongTransitionDirection * SlideDistance * (bIncoming ? 1.0f - Ease : -Ease);
		for (int32 Index = 0; Index < SongCards.Num(); ++Index)
		{
			SongCards[Index]->SetRenderTranslation(SongCardTranslations[Index] + FVector2D(Translation, 0));
			SongCards[Index]->SetRenderOpacity(SongCardOpacities[Index] * Opacity);
		}
		if (Alpha == 1.0f)
		{
			bSongTransitionActive = false;
			RestoreSongCards();
			if (PendingSongSteps != 0)
			{
				const double NextStartSeconds = SongTransitionStartSeconds + TransitionDurationSeconds;
				const int32 Direction = PendingSongSteps > 0 ? 1 : -1;
				PendingSongSteps -= Direction;
				QueueSongChange(Direction);
				SongTransitionStartSeconds = NextStartSeconds;
			}
		}
		else break;
	}
}

void UGuitarScreenWidget::RestoreSongCards()
{
	for (int32 Index = 0; Index < SongCards.Num(); ++Index)
	{
		SongCards[Index]->SetRenderTranslation(SongCardTranslations[Index]);
		SongCards[Index]->SetRenderOpacity(SongCardOpacities[Index]);
	}
}

void UGuitarScreenWidget::FinishSongTransition()
{
	// 切换中立即开始时，选中用户已经请求的目标曲目，再归位卡片
	if (bSongTransitionActive && !Songs.IsEmpty())
	{
		const int32 Steps = (bSongTransitionCommitted ? 0 : SongTransitionDirection) + PendingSongSteps;
		SelectedSongIndex = ((SelectedSongIndex + Steps) % Songs.Num() + Songs.Num()) % Songs.Num();
		RefreshSongCards();
	}
	bSongTransitionActive = false;
	PendingSongSteps = 0;
	RestoreSongCards();
}
