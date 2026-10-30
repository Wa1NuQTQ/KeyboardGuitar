#include "GuitarCarryMotion.h"

namespace
{
	// 过渡中段让琴中心离开躯干；这是当前人物/吉他的呈现轨迹，不是碰撞求解器。
	constexpr double MinimumSideRadiusCm = 45.0;
	constexpr double MinimumArcOffsetCm = 20.0;
}

FTransform GuitarCarry::CalculateTransform(float Progress01,
	const FTransform& FrontGuitarToMesh, const FTransform& CarryGuitarToMesh,
	const FVector& BoundsCenter, const FVector& TorsoPivot)
{
	const double Alpha = FMath::Clamp(static_cast<double>(Progress01), 0.0, 1.0);
	if (Alpha <= 0.0)
	{
		return CarryGuitarToMesh;
	}
	if (Alpha >= 1.0)
	{
		return FrontGuitarToMesh;
	}

	// 模型原点在琴体下方，按几何中心绕行才能保证不同朝向下的离身间距。
	const FVector CarryCenter = CarryGuitarToMesh.TransformPosition(BoundsCenter);
	const FVector FrontCenter = FrontGuitarToMesh.TransformPosition(BoundsCenter);
	const FVector CarryOffset = CarryCenter - TorsoPivot;
	const FVector FrontOffset = FrontCenter - TorsoPivot;
	const double CarryRadius = CarryOffset.Size2D();
	const double FrontRadius = FrontOffset.Size2D();
	const double CarryAngle = FMath::Atan2(CarryOffset.Y, CarryOffset.X);
	const double FrontAngle = FMath::Atan2(FrontOffset.Y, FrontOffset.X);

	double AngleDelta = FMath::FindDeltaAngleRadians(CarryAngle, FrontAngle);
	const double OtherDelta = AngleDelta >= 0.0 ? AngleDelta - 2.0 * UE_DOUBLE_PI : AngleDelta + 2.0 * UE_DOUBLE_PI;
	if (FMath::Cos(CarryAngle + OtherDelta * 0.5) < FMath::Cos(CarryAngle + AngleDelta * 0.5))
	{
		AngleDelta = OtherDelta;
	}

	const double MiddleRadius = (CarryRadius + FrontRadius) * 0.5;
	const double ArcOffset = FMath::Max(MinimumArcOffsetCm, MinimumSideRadiusCm - MiddleRadius);
	const double Radius = FMath::Lerp(CarryRadius, FrontRadius, Alpha) + FMath::Sin(UE_DOUBLE_PI * Alpha) * ArcOffset;
	const double Angle = CarryAngle + AngleDelta * Alpha;
	const FVector Center = TorsoPivot + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle),
		FMath::Lerp(CarryOffset.Z, FrontOffset.Z, Alpha));

	const FQuat Rotation = FQuat::Slerp(CarryGuitarToMesh.GetRotation(), FrontGuitarToMesh.GetRotation(), Alpha).GetNormalized();
	const FVector Scale = FMath::Lerp(CarryGuitarToMesh.GetScale3D(), FrontGuitarToMesh.GetScale3D(), Alpha);
	const FVector Origin = Center - Rotation.RotateVector(BoundsCenter * Scale);
	return FTransform(Rotation, Origin, Scale);
}
