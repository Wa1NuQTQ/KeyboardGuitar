#include "GuitarTabWidget.h"
#include "GuitarChordLabel.h"
#include "GuitarTabStyle.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Practice/GuitarSong.h"
#include "Rendering/DrawElements.h"
#include "Layout/Clipping.h"
#include "Styling/CoreStyle.h"

void UGuitarTabWidget::Configure(UGuitarPracticeSessionComponent* InSession)
{
	Session = InSession;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetClipping(EWidgetClipping::ClipToBounds);
	ForceVolatile(true);
}

int32 UGuitarTabWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
	Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bEnabled);
	const FVector2D Size = Geometry.GetLocalSize();
	const float HitX = Size.X * 0.14f;
	const float Top = 64.0f;
	const float LabelLeft = 25.0f;
	const float LineLeft = 80.0f;
	const float Right = Size.X - 26.0f;
	const float Spacing = FMath::Max(10.0f, (Size.Y - Top - 30.0f) / 5.0f);
	const FLinearColor Ink = GuitarUI::GetTabColors(ChordType).Ink;
	const FLinearColor Muted = Ink.CopyWithNewOpacity(0.25f);
	auto Line = [&](FVector2D A, FVector2D B, FLinearColor Color, float Width)
	{
		TArray<FVector2D> Points = {A, B};
		FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Width);
	};
	auto Text = [&](FVector2D Position, const FString& Value, int32 FontSize, FLinearColor Color)
	{
		FSlateDrawElement::MakeText(Elements, Layer + 3, Geometry.ToPaintGeometry(FVector2D(150, 35), FSlateLayoutTransform(Position)),
			Value, FCoreStyle::GetDefaultFontStyle("Regular", FontSize), ESlateDrawEffect::None, Color);
	};
	for (int32 String = 0; String < Guitar::StringCount; ++String)
	{
		const float Y = Top + String * Spacing;
		Line({LineLeft, Y}, {Right, Y}, Muted, 1.0f + String * 0.15f);
		Text({LabelLeft, Y - 18}, FString::FromInt(String + 1), 26, Ink);
	}
	Line({HitX, Top - 10}, {HitX, Top + 5 * Spacing + 14}, Ink, 3.0f);
	if (!Session || !Session->GetSong()) return Layer + 3;
	const UGuitarSong* Song = Session->GetSong();
	const double Time = Session->GetSongTimeSeconds();
	const float PixelsPerSecond = (Size.X - HitX - 40.0f) / 5.0f;
	// 节拍线与音符使用同一时钟，暂停时谱面自然冻结
	const double BeatSeconds = 60.0 / Song->BeatsPerMinute;
	const double BeatOrigin = Song->GetBeatOriginSeconds();
	for (int32 Beat = FMath::FloorToInt((Time - BeatOrigin) / BeatSeconds); BeatOrigin + Beat * BeatSeconds < Time + 5.5; ++Beat)
	{
		const float X = HitX + (BeatOrigin + Beat * BeatSeconds - Time) * PixelsPerSecond;
		if (X < LineLeft || X > Right) continue;
		Line({X, Top - 8}, {X, Top + 5 * Spacing + 8}, Ink.CopyWithNewOpacity(Beat % 4 == 0 ? 0.18f : 0.08f), 1);
	}
	// 和弦名与该段首次拨弦共用横坐标，经过判定线后继续移出，不固定在左侧。
	// 单独裁剪标题区域，移出时不遮住固定弦号。
	Elements.PushClip(FSlateClippingZone(Geometry.MakeChild(FVector2D(Right - LineLeft, Top - 8), FSlateLayoutTransform(FVector2D(LineLeft, 0)))));
	int32 PluckIndex = 0;
	for (int32 ChangeIndex = -1; ChangeIndex < Song->ChordChanges.Num(); ++ChangeIndex)
	{
		const double Start = ChangeIndex < 0 ? 0.0 : Song->ChordChanges[ChangeIndex].StartTimeSeconds;
		const double End = Song->ChordChanges.IsValidIndex(ChangeIndex + 1)
			? Song->ChordChanges[ChangeIndex + 1].StartTimeSeconds : Song->DurationSeconds;
		while (Song->Plucks.IsValidIndex(PluckIndex) && Song->Plucks[PluckIndex].StartTimeSeconds < Start) ++PluckIndex;
		if (!Song->Plucks.IsValidIndex(PluckIndex) || Song->Plucks[PluckIndex].StartTimeSeconds >= End) continue;
		const double FirstPluckTime = Song->Plucks[PluckIndex].StartTimeSeconds;
		const float X = HitX + (FirstPluckTime - Time) * PixelsPerSecond;
		if (X < LineLeft - 300.0f || X > Right) continue;
		const FName ChordId = ChangeIndex < 0 ? NAME_None : Song->ChordChanges[ChangeIndex].ChordId;
		const FLinearColor Color = GuitarUI::GetChordColor(ChordId);
		Text({X, 2}, GuitarUI::MakeChordLabel(ChordId), 22, Color);
		// 和弦进入右侧预览区就提示类型键，不等到临近拨弦；选对类型或已持有所需和弦时隐藏。
		if (Time <= FirstPluckTime && PlayedChord != ChordId)
			Text({X, 32}, GuitarUI::MakeTypeSwitchHint(ChordId, ChordType), 16, Color);
	}
	Elements.PopClip();
	const TArray<EGuitarJudgement>& States = Session->GetJudgements();
	for (int32 Index = 0; Index < Song->Plucks.Num(); ++Index)
	{
		const FGuitarPluckEvent& Note = Song->Plucks[Index];
		const float X = HitX + (Note.StartTimeSeconds - Time) * PixelsPerSecond;
		if (X < LineLeft + 14 || X > Right - 14) continue;
		FLinearColor Color = Ink;
		if (States.IsValidIndex(Index))
		{
			if (States[Index] == EGuitarJudgement::Perfect || States[Index] == EGuitarJudgement::Good) Color = FLinearColor(0.26f, 0.58f, 0.34f);
			else if (States[Index] != EGuitarJudgement::Pending) Color = FLinearColor(0.72f, 0.28f, 0.2f);
		}
		// 音符只表示拨弦位置，不再显示容易与键盘弦号混淆的品位数字
		const float Y = Top + Note.StringIndex * Spacing;
		constexpr float HalfSize = 8.0f;
		Line({X - HalfSize, Y - HalfSize}, {X + HalfSize, Y + HalfSize}, Color, 3.0f);
		Line({X - HalfSize, Y + HalfSize}, {X + HalfSize, Y - HalfSize}, Color, 3.0f);
	}
	return Layer + 3;
}
