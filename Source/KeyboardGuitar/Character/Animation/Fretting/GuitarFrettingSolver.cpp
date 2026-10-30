#include "GuitarFrettingSolver.h"

#include "Instrument/GuitarTypes.h"

namespace
{
	constexpr int32 FingerCount = 5;
	constexpr double SurfaceOffsetCm = 0.25;
	const FVector FretboardOutward(0.0, 1.0, 0.0);
	const FVector AcrossStrings(-1.0, 0.0, 0.0);

	struct FFingerContacts
	{
		int32 ContactCount = 0;
		int32 Fret = 0;
		int32 LastString = 0;
		FVector Tip = FVector::ZeroVector;
		FVector Points[Guitar::StringCount];
	};

	FGuitarFingerTarget& FingerAt(FGuitarFrettingPose& Pose, int32 FingerIndex)
	{
		switch (FingerIndex)
		{
		case 0:
			return Pose.Index;
		case 1:
			return Pose.Middle;
		case 2:
			return Pose.Ring;
		case 3:
			return Pose.Little;
		default:
			return Pose.Thumb;
		}
	}

	double AngleDegrees(const FVector& FirstDirection, const FVector& SecondDirection)
	{
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
			FVector::DotProduct(FirstDirection.GetSafeNormal(), SecondDirection.GetSafeNormal()), -1.0, 1.0)));
	}

	struct FFingerBend
	{
		FVector Middle;
		FVector Distal;
		double MiddleAngle;
		double DistalAngle;
		double ReachError;
		bool bReverse;
	};

	FFingerBend CalculateBend(const FVector& FingerRoot, const FVector& TipPosition, const FVector& SegmentLengths,
		const FVector& TipDirection, const FVector& BendNormal)
	{
		FFingerBend Bend;
		Bend.Distal = TipPosition - TipDirection * SegmentLengths.Z;
		const FVector Delta = Bend.Distal - FingerRoot;
		const double Distance = Delta.Length();
		Bend.ReachError = FMath::Max3(0.0, Distance - SegmentLengths.X - SegmentLengths.Y + 0.001,
			FMath::Abs(SegmentLengths.X - SegmentLengths.Y) + 0.001 - Distance);
		const double ClampedDistance = FMath::Clamp(Distance,
			FMath::Abs(SegmentLengths.X - SegmentLengths.Y) + 0.001, SegmentLengths.X + SegmentLengths.Y - 0.001);
		const FVector Axis = Delta.GetSafeNormal();
		const FVector Side = (BendNormal - Axis * FVector::DotProduct(BendNormal, Axis)).GetSafeNormal();
		const double Along = (SegmentLengths.X * SegmentLengths.X - SegmentLengths.Y * SegmentLengths.Y
			+ ClampedDistance * ClampedDistance) / (2.0 * ClampedDistance);
		Bend.Middle = FingerRoot + Axis * Along
			+ Side * FMath::Sqrt(FMath::Max(0.0, SegmentLengths.X * SegmentLengths.X - Along * Along));
		Bend.MiddleAngle = AngleDegrees(Bend.Middle - FingerRoot, Bend.Distal - Bend.Middle);
		Bend.DistalAngle = AngleDegrees(Bend.Distal - Bend.Middle, TipDirection);
		Bend.bReverse = FVector::DotProduct(FVector::CrossProduct(Bend.Middle - FingerRoot, Bend.Distal - Bend.Middle),
			FVector::CrossProduct(Bend.Distal - Bend.Middle, TipDirection)) < -0.001;
		return Bend;
	}

	// 给定末节方向后，前三节仍由同一弯曲平面内的两骨三角形求解。
	double FitFinger(const FVector& FingerRoot, const FVector& TipPosition, const FVector& SegmentLengths,
		const FVector& BendNormal, const FGuitarFingerGeometry& Geometry, bool bBarre,
		const FFingerContacts& Contacts, FGuitarFingerTarget& InOutTarget)
	{
		const FVector RootToTipAxis = (TipPosition - FingerRoot).GetSafeNormal();
		const FVector BendSideAxis = (BendNormal - RootToTipAxis * FVector::DotProduct(BendNormal, RootToTipAxis)).GetSafeNormal();
		double BestCost = TNumericLimits<double>::Max();
		FVector BestPole = FingerRoot;
		FVector BestDirection = -BendNormal;
		const int32 FirstAngle = bBarre ? -12 : 30;
		const int32 LastAngle = bBarre ? 12 : 145;
		const bool bThumb = BendNormal.Y < 0.0;
		double PreferredMiddleAngle = bThumb ? 50.0 : 80.0;
		double PreferredDistalAngle = bThumb ? 35.0 : 45.0;
		if (bBarre)
		{
			PreferredMiddleAngle = 65.0;
			PreferredDistalAngle = 4.0;
		}
		for (int32 Degrees = FirstAngle; Degrees <= LastAngle; Degrees += bBarre ? 3 : 5)
		{
			const double Radians = FMath::DegreesToRadians(static_cast<double>(Degrees));
			const FVector Direction = bBarre
				? AcrossStrings * FMath::Cos(Radians) - BendNormal * FMath::Sin(Radians)
				: RootToTipAxis * FMath::Cos(Radians) - BendSideAxis * FMath::Sin(Radians);
			const FFingerBend Bend = CalculateBend(FingerRoot, TipPosition, SegmentLengths, Direction, BendNormal);
			double Cost = 1000.0 * Bend.ReachError * Bend.ReachError;
			Cost += FMath::Square((Bend.MiddleAngle - PreferredMiddleAngle) / 35.0);
			Cost += FMath::Square((Bend.DistalAngle - PreferredDistalAngle) / (bBarre ? 12.0 : 25.0));
			Cost += Bend.bReverse ? 100.0 : 0.0;
			Cost += FMath::Square(FMath::Max(0.0, Bend.MiddleAngle - 120.0)) + FMath::Square(FMath::Max(0.0, Bend.DistalAngle - 85.0));
			if (bBarre)
			{
				// 横按约束整段指腹，而不是只让指尖碰到最远的一根弦。
				for (int32 ContactIndex = 0; ContactIndex < Contacts.ContactCount; ++ContactIndex)
				{
					const FVector ContactPosition = Contacts.Points[ContactIndex] + BendNormal * SurfaceOffsetCm;
					const double DistanceToFinger = FMath::Min3(
						FMath::PointDistToSegment(ContactPosition, FingerRoot, Bend.Middle),
						FMath::PointDistToSegment(ContactPosition, Bend.Middle, Bend.Distal),
						FMath::PointDistToSegment(ContactPosition, Bend.Distal, TipPosition));
					Cost += 35.0 * DistanceToFinger * DistanceToFinger;
				}
			}
			if (Cost < BestCost)
			{
				BestCost = Cost;
				BestPole = Bend.Middle;
				BestDirection = Direction;
			}
		}
		const FQuat SeedRotation = InOutTarget.Tip.GetRotation();
		const FVector SeedDirection = SeedRotation.RotateVector(Geometry.TipAxis).GetSafeNormal();
		InOutTarget.Tip.SetRotation((FQuat::FindBetweenNormals(SeedDirection, BestDirection) * SeedRotation).GetNormalized());
		InOutTarget.Tip.SetLocation(TipPosition);
		InOutTarget.Bend = BestPole;
		return BestCost;
	}
}

bool GuitarFretting::SolveChord(const FGuitarChord& Chord, const FGuitarFretboard& Board,
	const TArray<FGuitarFingerGeometry>& Geometry, const FGuitarFrettingPose& OpenGrip,
	const FGuitarFrettingPose& BarreGrip, FGuitarFrettingPose& OutPose)
{
	if (Geometry.Num() != FingerCount || Chord.StringFingerings.Num() != Guitar::StringCount)
	{
		return false;
	}

	// 先按手指汇总所有按弦点；同指同品的多个接触点组成横按约束。
	FFingerContacts FingerContacts[4];
	int32 BarreFingerIndex = INDEX_NONE;
	for (int32 StringIndex = 0; StringIndex < Guitar::StringCount; ++StringIndex)
	{
		const FGuitarStringFingering& Fingering = Chord.StringFingerings[StringIndex];
		if (Fingering.bMuted || Fingering.Fret == 0)
		{
			continue;
		}
		const int32 FingerIndex = static_cast<int32>(Fingering.Finger) - 1;
		if (FingerIndex < 0 || FingerIndex >= 4
			|| (FingerContacts[FingerIndex].ContactCount && FingerContacts[FingerIndex].Fret != Fingering.Fret))
		{
			return false;
		}
		FFingerContacts& Contacts = FingerContacts[FingerIndex];
		Contacts.Fret = Fingering.Fret;
		Contacts.LastString = FMath::Max(Contacts.LastString, StringIndex);
		Contacts.Points[Contacts.ContactCount++] = Board.PressPosition(StringIndex, Fingering.Fret);
		if (Contacts.ContactCount > 1
			&& (BarreFingerIndex == INDEX_NONE || Contacts.ContactCount > FingerContacts[BarreFingerIndex].ContactCount))
		{
			BarreFingerIndex = FingerIndex;
		}
	}

	// 选择开放/横按握姿作为起点，再按接触高度估计手腕位置。
	const FGuitarFrettingPose& GripTemplate = BarreFingerIndex == INDEX_NONE ? OpenGrip : BarreGrip;
	const FGuitarFingerTarget* TemplateFingerTargets[] =
	{
		&GripTemplate.Index, &GripTemplate.Middle, &GripTemplate.Ring, &GripTemplate.Little
	};
	const double WristScale = GripTemplate.Wrist.GetScale3D().GetAbsMax();
	if (WristScale <= UE_SMALL_NUMBER || Geometry[0].SegmentLengthsCm.GetMin() <= UE_SMALL_NUMBER)
	{
		return false;
	}
	FVector WristSeedPosition = GripTemplate.Wrist.GetLocation();
	double WristHeightSum = 0.0;
	int32 ActiveFingerCount = 0;
	for (int32 FingerIndex = 0; FingerIndex < 4; ++FingerIndex)
	{
		FFingerContacts& Contacts = FingerContacts[FingerIndex];
		if (!Contacts.ContactCount)
		{
			continue;
		}
		Contacts.Tip = Board.PressPosition(Contacts.LastString, Contacts.Fret)
			+ TemplateFingerTargets[FingerIndex]->ContactOffsetCm;
		if (Contacts.ContactCount > 1)
		{
			Contacts.Tip += AcrossStrings * 0.25;
		}
		const FVector FingerRootOffset = GripTemplate.Wrist.GetRotation().RotateVector(Geometry[FingerIndex].RootInWrist * WristScale);
		WristHeightSum += Contacts.Tip.Z - FingerRootOffset.Z;
		++ActiveFingerCount;
	}
	if (!ActiveFingerCount)
	{
		OutPose = OpenGrip;
		return true;
	}
	WristSeedPosition.Z = WristHeightSum / ActiveFingerCount - 1.0;
	if (BarreFingerIndex != INDEX_NONE)
	{
		const FVector SegmentLengths = Geometry[BarreFingerIndex].SegmentLengthsCm * WristScale;
		const FVector IdealMiddleJoint = FingerContacts[BarreFingerIndex].Tip - AcrossStrings * (SegmentLengths.Y + SegmentLengths.Z);
		const FVector FingerRootOffset = GripTemplate.Wrist.GetRotation().RotateVector(Geometry[BarreFingerIndex].RootInWrist * WristScale);
		const FVector RootSide(1.5, -0.4, 0.0);
		const double RootAlongNeck = -FMath::Sqrt(FMath::Max(0.0, SegmentLengths.X * SegmentLengths.X - RootSide.SizeSquared()));
		WristSeedPosition = IdealMiddleJoint + RootSide + FVector(0.0, 0.0, RootAlongNeck) - FingerRootOffset;
	}

	// 每个候选只改变手腕的三轴位移/旋转，再综合五指可达性、弯曲和横按接触评分。
	auto EvaluateWristCandidate = [&](const double* WristOffsets, FGuitarFrettingPose* OutCandidatePose)
	{
		FGuitarFrettingPose CandidatePose = GripTemplate;
		const FVector WristPosition = WristSeedPosition + FVector(WristOffsets[0], WristOffsets[1], WristOffsets[2]);
		const FQuat WristRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(WristOffsets[5]))
			* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(WristOffsets[4]))
			* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(WristOffsets[3])) * GripTemplate.Wrist.GetRotation();
		CandidatePose.Wrist = FTransform(WristRotation, WristPosition, GripTemplate.Wrist.GetScale3D());
		const FTransform TemplateWristInverse = GripTemplate.Wrist.Inverse();
		CandidatePose.Elbow = GripTemplate.Elbow + (WristPosition - GripTemplate.Wrist.GetLocation()) * 0.3;
		double CandidateCost = 0.001 * (WristOffsets[3] * WristOffsets[3] + WristOffsets[4] * WristOffsets[4] + WristOffsets[5] * WristOffsets[5]);
		CandidateCost += 0.03 * (WristOffsets[0] * WristOffsets[0] + WristOffsets[1] * WristOffsets[1] + WristOffsets[2] * WristOffsets[2]);
		for (int32 FingerIndex = 0; FingerIndex < FingerCount; ++FingerIndex)
		{
			FGuitarFingerTarget& FingerTarget = FingerAt(CandidatePose, FingerIndex);
			FingerTarget.Tip = FingerTarget.Tip * TemplateWristInverse * CandidatePose.Wrist;
			const FVector FingerRoot = CandidatePose.Wrist.TransformPosition(Geometry[FingerIndex].RootInWrist);
			FVector TipPosition = FingerTarget.Tip.GetLocation();
			if (FingerIndex < 4 && FingerContacts[FingerIndex].ContactCount)
			{
				TipPosition = FingerContacts[FingerIndex].Tip;
			}
			else if (FingerIndex == 4)
			{
				// 拇指支撑琴颈背面，不跟着某根弦移动。
				TipPosition = Board.PressPosition(2, FMath::Max(1, FingerContacts[BarreFingerIndex == INDEX_NONE ? 0 : BarreFingerIndex].Fret));
				TipPosition.X = (Board.Nut1.X + Board.Nut6.X) * 0.5;
				TipPosition.Y -= 3.1;
				TipPosition.Z = WristPosition.Z + 3.0;
			}
			else
			{
				TipPosition += FVector(0.4, 0.8, 0.0);
			}
			const FFingerContacts NoContacts {};
			const bool bBarre = FingerIndex < 4 && FingerContacts[FingerIndex].ContactCount > 1;
			const double FingerCost = FitFinger(FingerRoot, TipPosition, Geometry[FingerIndex].SegmentLengthsCm * WristScale,
				FingerIndex == 4 ? -FretboardOutward : FretboardOutward, Geometry[FingerIndex], bBarre,
				FingerIndex < 4 ? FingerContacts[FingerIndex] : NoContacts, FingerTarget);
			double FingerWeight = 1.0;
			if (FingerIndex == 4)
			{
				FingerWeight = 0.35;
			}
			else if (!FingerContacts[FingerIndex].ContactCount)
			{
				FingerWeight = 0.15;
			}
			CandidateCost += FingerCost * FingerWeight;
		}
		if (OutCandidatePose)
		{
			*OutCandidatePose = CandidatePose;
		}
		return CandidateCost;
	};

	// 六个参数依次为位移 XYZ（cm）、旋转 XYZ（度）；逐轴搜索并逐轮缩小步长。
	double BestWristOffsets[6] = {};
	double BestPoseCost = EvaluateWristCandidate(BestWristOffsets, nullptr);
	const double WristOffsetLimits[6] = { 5.0, 3.5, 6.0, 30.0, 55.0, 40.0 };
	const double InitialSearchSteps[6] = { 1.5, 1.0, 2.0, 12.0, 18.0, 12.0 };
	for (int32 Round = 0; Round < 6; ++Round)
	{
		for (int32 Sweep = 0; Sweep < 3; ++Sweep)
		{
			for (int32 ParameterIndex = 0; ParameterIndex < 6; ++ParameterIndex)
			{
				const double OriginalOffset = BestWristOffsets[ParameterIndex];
				double BestOffset = OriginalOffset;
				for (double Sign : { -1.0, 1.0 })
				{
					BestWristOffsets[ParameterIndex] = FMath::Clamp(
						OriginalOffset + Sign * InitialSearchSteps[ParameterIndex] * FMath::Pow(0.5, Round),
						-WristOffsetLimits[ParameterIndex], WristOffsetLimits[ParameterIndex]);
					const double CandidateCost = EvaluateWristCandidate(BestWristOffsets, nullptr);
					if (CandidateCost < BestPoseCost)
					{
						BestPoseCost = CandidateCost;
						BestOffset = BestWristOffsets[ParameterIndex];
					}
				}
				BestWristOffsets[ParameterIndex] = BestOffset;
			}
		}
	}
	EvaluateWristCandidate(BestWristOffsets, &OutPose);
	return true;
}

void GuitarFretting::ConstrainFingerBends(FGuitarFrettingPose& Pose, const TArray<FGuitarFingerGeometry>& Geometry,
	bool bKeepBendsPlanar)
{
	if (Geometry.Num() != FingerCount)
	{
		return;
	}
	const double WristScale = Pose.Wrist.GetScale3D().GetAbsMax();
	for (int32 FingerIndex = 0; FingerIndex < FingerCount; ++FingerIndex)
	{
		FGuitarFingerTarget& FingerTarget = FingerAt(Pose, FingerIndex);
		const FVector FingerRoot = Pose.Wrist.TransformPosition(Geometry[FingerIndex].RootInWrist);
		const FVector TipPosition = FingerTarget.Tip.GetLocation();
		const FVector SegmentLengths = Geometry[FingerIndex].SegmentLengthsCm * WristScale;
		const FVector BendNormal = (FingerTarget.Bend - FingerRoot).GetSafeNormal();
		const FQuat TipRotation = FingerTarget.Tip.GetRotation();
		const FVector DesiredDirection = TipRotation.RotateVector(Geometry[FingerIndex].TipAxis).GetSafeNormal();
		const FFingerBend CurrentBend = CalculateBend(FingerRoot, TipPosition, SegmentLengths, DesiredDirection, BendNormal);
		const FVector FirstPlane = FVector::CrossProduct(CurrentBend.Middle - FingerRoot,
			CurrentBend.Distal - CurrentBend.Middle).GetSafeNormal();
		const FVector SecondPlane = FVector::CrossProduct(CurrentBend.Distal - CurrentBend.Middle,
			DesiredDirection).GetSafeNormal();
		const bool bOutOfPlane = bKeepBendsPlanar && !FirstPlane.IsNearlyZero() && !SecondPlane.IsNearlyZero()
			&& FVector::DotProduct(FirstPlane, SecondPlane) < FMath::Cos(FMath::DegreesToRadians(15.0));
		if (!CurrentBend.bReverse && CurrentBend.ReachError < 0.001
			&& CurrentBend.MiddleAngle <= 120.0 && CurrentBend.DistalAngle <= 85.0 && !bOutOfPlane)
		{
			continue;
		}

		// 只纠正过渡中的不可用弯曲：保留指尖位置，寻找最接近当前朝向的有效末节方向和 Pole。
		const FVector RootToTipAxis = (TipPosition - FingerRoot).GetSafeNormal();
		const FVector BendSideAxis = (BendNormal - RootToTipAxis * FVector::DotProduct(BendNormal, RootToTipAxis)).GetSafeNormal();
		double BestDirectionChange = TNumericLimits<double>::Max();
		FVector BestDirection = DesiredDirection;
		FVector BestPole = FingerTarget.Bend;
		for (int32 Degrees = 0; Degrees <= 170; ++Degrees)
		{
			const double Radians = FMath::DegreesToRadians(static_cast<double>(Degrees));
			const FVector Direction = RootToTipAxis * FMath::Cos(Radians) - BendSideAxis * FMath::Sin(Radians);
			const FFingerBend Bend = CalculateBend(FingerRoot, TipPosition, SegmentLengths, Direction, BendNormal);
			if (Bend.bReverse || Bend.ReachError >= 0.001 || Bend.MiddleAngle > 120.0 || Bend.DistalAngle > 85.0)
			{
				continue;
			}
			const double DirectionChange = AngleDegrees(Direction, DesiredDirection);
			if (DirectionChange < BestDirectionChange)
			{
				BestDirectionChange = DirectionChange;
				BestDirection = Direction;
				BestPole = Bend.Middle;
			}
		}
		FingerTarget.Tip.SetRotation((FQuat::FindBetweenNormals(DesiredDirection, BestDirection) * TipRotation).GetNormalized());
		FingerTarget.Bend = BestPole;
	}
}
