#include "GuitarFrettingPose.h"

namespace
{
	void BlendFinger(FGuitarFingerTarget& Current, const FGuitarFingerTarget& Target, float Alpha)
	{
		FTransform Blended;
		Blended.Blend(Current.Tip, Target.Tip, Alpha);
		Current.Tip = Blended;
		Current.Bend = FMath::Lerp(Current.Bend, Target.Bend, Alpha);
	}

	void TransformFinger(FGuitarFingerTarget& Finger, const FTransform& LocalToTarget)
	{
		Finger.Tip = Finger.Tip * LocalToTarget;
		Finger.Bend = LocalToTarget.TransformPosition(Finger.Bend);
	}
}

void FGuitarFrettingPose::BlendTowards(const FGuitarFrettingPose& Target, float BlendAlpha)
{
	FTransform BlendedWrist;
	BlendedWrist.Blend(Wrist, Target.Wrist, BlendAlpha);
	Wrist = BlendedWrist;
	Elbow = FMath::Lerp(Elbow, Target.Elbow, BlendAlpha);
	BlendFinger(Index, Target.Index, BlendAlpha);
	BlendFinger(Middle, Target.Middle, BlendAlpha);
	BlendFinger(Ring, Target.Ring, BlendAlpha);
	BlendFinger(Little, Target.Little, BlendAlpha);
	BlendFinger(Thumb, Target.Thumb, BlendAlpha);
}

FGuitarFrettingPose FGuitarFrettingPose::GetTransformed(const FTransform& LocalToTarget) const
{
	FGuitarFrettingPose Result = *this;
	Result.Wrist = Wrist * LocalToTarget;
	Result.Elbow = LocalToTarget.TransformPosition(Elbow);
	TransformFinger(Result.Index, LocalToTarget);
	TransformFinger(Result.Middle, LocalToTarget);
	TransformFinger(Result.Ring, LocalToTarget);
	TransformFinger(Result.Little, LocalToTarget);
	TransformFinger(Result.Thumb, LocalToTarget);
	return Result;
}
