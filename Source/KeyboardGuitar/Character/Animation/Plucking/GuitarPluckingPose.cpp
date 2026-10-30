#include "GuitarPluckingPose.h"

#include "Character/Animation/Fretting/GuitarFrettingSolver.h"
#include "GuitarPluckingDefinition.h"
#include "Instrument/GuitarTypes.h"

namespace
{
	constexpr float AttackSeconds = 0.035f;
	constexpr float RecoverySeconds = 0.14f;
	constexpr float ThumbMoveSeconds = 0.05f;
	constexpr int32 FingerForString[Guitar::StringCount] = {3, 2, 1, 0, 0, 0};

	FGuitarFingerTarget MakePluckingTarget(const FGuitarFingerTarget& Ready,
		const FGuitarFretboard& Board, float StringPosition, float Stroke01, float Lift01, bool bThumb)
	{
		const double StringAlpha = StringPosition / (Guitar::StringCount - 1);
		const FVector Nut = FMath::Lerp(Board.Nut1, Board.Nut6, StringAlpha);
		const FVector Bridge = FMath::Lerp(Board.Bridge1, Board.Bridge6, StringAlpha);
		const double AlongString = (Nut.Z - Ready.Tip.GetLocation().Z) / (Nut.Z - Bridge.Z);
		const FVector StringPoint = FMath::Lerp(Nut, Bridge, AlongString);
		const double Direction = bThumb ? 1.0 : -1.0;
		// 拇指跨弦后的跟进短一些，给食指留出空间，避免拇指过度伸直。
		const double StrokeDistanceCm = bThumb ? 0.4 : 0.8;
		// 三指准备时贴近琴弦，拇指悬空；拨动后抬指返回，避免回程再次扫弦。
		const double SurfaceClearanceCm = bThumb ? FMath::Lerp(0.9, 0.25, Stroke01) : 0.25;
		const FVector Tip = StringPoint + FVector(Direction * StrokeDistanceCm * Stroke01,
			SurfaceClearanceCm + 0.7 * Lift01, 0.0);
		FGuitarFingerTarget Result = Ready;
		Result.Tip.SetLocation(Tip);
		Result.Bend += (Tip - Ready.Tip.GetLocation()) * 0.35;
		return Result;
	}
}

FGuitarFrettingPose FGuitarPluckingState::EvaluatePose(const UGuitarPluckingDefinition& Definition,
	const FGuitarFretboard& Board) const
{
	FGuitarFrettingPose Targets;
	Targets.Wrist = Definition.Wrist;
	Targets.Elbow = Definition.Elbow;
	Targets.Index = MakePluckingTarget(Definition.Index, Board, 2.0f, Pose.IndexCurl01, GetReturnLift(1), false);
	Targets.Middle = MakePluckingTarget(Definition.Middle, Board, 1.0f, Pose.MiddleCurl01, GetReturnLift(2), false);
	Targets.Ring = MakePluckingTarget(Definition.Ring, Board, 0.0f, Pose.RingCurl01, GetReturnLift(3), false);
	Targets.Little = Definition.Little;
	Targets.Thumb = MakePluckingTarget(Definition.Thumb, Board, Pose.ThumbStringPosition, Pose.ThumbCurl01, GetReturnLift(0), true);
	// 左右手使用同样的腕/末端几何约束；右手不进行和弦品位搜索。
	GuitarFretting::ConstrainFingerBends(Targets, Definition.FingerGeometry, true);
	return Targets;
}

void FGuitarPluckingState::PluckString(int32 StringIndex)
{
	if (StringIndex < 0 || StringIndex >= Guitar::StringCount)
	{
		return;
	}

	const float CurrentCurls[] = {Pose.ThumbCurl01, Pose.IndexCurl01, Pose.MiddleCurl01, Pose.RingCurl01};
	const int32 FingerIndex = FingerForString[StringIndex];
	FFingerStroke& Stroke = Strokes[FingerIndex];
	// 连续重拨从当前值开始，避免在上一动作尚未结束时突然跳回准备姿势。
	Stroke.StartCurl01 = CurrentCurls[FingerIndex];
	Stroke.StartLift01 = Stroke.ReturnLift01;
	Stroke.ElapsedSeconds = 0.0f;
	Stroke.bActive = true;
	if (FingerIndex == 0)
	{
		TargetThumbStringPosition = static_cast<float>(StringIndex);
	}
}

void FGuitarPluckingState::Update(float DeltaSeconds)
{
	const float StepSeconds = FMath::Max(DeltaSeconds, 0.0f);
	// 一帧跨过拨弦/恢复边界时，分别推进移弦和回位，避免低帧率跳过拨弦目标。
	const float ThumbAttackStepSeconds = Strokes[0].bActive
		? FMath::Clamp(AttackSeconds - Strokes[0].ElapsedSeconds, 0.0f, StepSeconds) : 0.0f;
	const float ThumbAttackAlpha = 1.0f - FMath::Exp(-3.0f * ThumbAttackStepSeconds / ThumbMoveSeconds);
	Pose.ThumbStringPosition = FMath::Lerp(Pose.ThumbStringPosition, TargetThumbStringPosition, ThumbAttackAlpha);

	float* Curls[] = {&Pose.ThumbCurl01, &Pose.IndexCurl01, &Pose.MiddleCurl01, &Pose.RingCurl01};
	for (int32 FingerIndex = 0; FingerIndex < UE_ARRAY_COUNT(Strokes); ++FingerIndex)
	{
		FFingerStroke& Stroke = Strokes[FingerIndex];
		if (!Stroke.bActive)
		{
			continue;
		}

		Stroke.ElapsedSeconds += StepSeconds;
		if (FingerIndex == 0 && Stroke.ElapsedSeconds >= AttackSeconds)
		{
			// 拨弦后平滑回第六弦；重拨会从当前值重新移向本次目标。
			TargetThumbStringPosition = 5.0f;
		}
		if (Stroke.ElapsedSeconds < AttackSeconds)
		{
			const float Alpha = FMath::SmoothStep(0.0f, 1.0f, Stroke.ElapsedSeconds / AttackSeconds);
			*Curls[FingerIndex] = FMath::Lerp(Stroke.StartCurl01, 1.0f, Alpha);
			Stroke.ReturnLift01 = FMath::Lerp(Stroke.StartLift01, 0.0f, Alpha);
		}
		else if (Stroke.ElapsedSeconds < AttackSeconds + RecoverySeconds)
		{
			const float Alpha = FMath::SmoothStep(0.0f, 1.0f, (Stroke.ElapsedSeconds - AttackSeconds) / RecoverySeconds);
			*Curls[FingerIndex] = 1.0f - Alpha;
			Stroke.ReturnLift01 = FMath::Sin(UE_PI * (Stroke.ElapsedSeconds - AttackSeconds) / RecoverySeconds);
		}
		else
		{
			*Curls[FingerIndex] = 0.0f;
			Stroke.ReturnLift01 = 0.0f;
			Stroke.bActive = false;
		}
	}

	// 在约 ThumbMoveSeconds 内完成 95% 回位，和帧率无关。
	const float ThumbAlpha = 1.0f - FMath::Exp(-3.0f * (StepSeconds - ThumbAttackStepSeconds) / ThumbMoveSeconds);
	Pose.ThumbStringPosition = FMath::Lerp(Pose.ThumbStringPosition, TargetThumbStringPosition, ThumbAlpha);
}

float FGuitarPluckingState::GetReturnLift(int32 FingerIndex) const
{
	return FingerIndex >= 0 && FingerIndex < UE_ARRAY_COUNT(Strokes) ? Strokes[FingerIndex].ReturnLift01 : 0.0f;
}

void FGuitarPluckingState::Reset()
{
	Pose = FGuitarPluckingPose();
	for (FFingerStroke& Stroke : Strokes)
	{
		Stroke = FFingerStroke();
	}
	TargetThumbStringPosition = Pose.ThumbStringPosition;
}
