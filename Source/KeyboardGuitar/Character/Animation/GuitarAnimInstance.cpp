#include "GuitarAnimInstance.h"

#include "Character/GuitarCharacter.h"
#include "Character/Animation/Fretting/GuitarFrettingDefinition.h"
#include "Character/Animation/Fretting/GuitarFrettingSolver.h"
#include "Character/Animation/Plucking/GuitarPluckingDefinition.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "Instrument/GuitarTypes.h"

namespace
{
	FGuitarFretboard ReadFretboard(const UStaticMeshComponent& GuitarMesh)
	{
		return {
			GuitarMesh.GetSocketTransform(TEXT("Fretboard_Nut_String1"), RTS_Component).GetLocation(),
			GuitarMesh.GetSocketTransform(TEXT("Fretboard_Nut_String6"), RTS_Component).GetLocation(),
			GuitarMesh.GetSocketTransform(TEXT("Fretboard_Bridge_String1"), RTS_Component).GetLocation(),
			GuitarMesh.GetSocketTransform(TEXT("Fretboard_Bridge_String6"), RTS_Component).GetLocation() };
	}
}

void UGuitarAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ResetFrettingState();
	PluckingState.Reset();
	PluckingPose = FGuitarPluckingPose();
	GuitarPoseAlpha = 0.0f;
}

void UGuitarAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* OwningPawn = TryGetPawnOwner();
	GroundSpeed = OwningPawn ? OwningPawn->GetVelocity().Size2D() : 0.0f;

	const AGuitarCharacter* Character = Cast<AGuitarCharacter>(OwningPawn);
	if (!Character || !FrettingDefinition)
	{
		ResetFrettingState();
		PluckingState.Reset();
		PluckingPose = FGuitarPluckingPose();
		GuitarPoseAlpha = 0.0f;
		return;
	}

	UpdateFrettingPose(*Character, DeltaSeconds);
	GuitarPoseAlpha = Character->GetGuitarPoseAlpha();
	UpdatePluckingPose(*Character, DeltaSeconds);
}

void UGuitarAnimInstance::UpdatePluckingPose(const AGuitarCharacter& Character, float DeltaSeconds)
{
	PluckingState.Update(DeltaSeconds);
	PluckingPose = PluckingState.GetPose();
	if (PluckingDefinition)
	{
		const UStaticMeshComponent* GuitarMesh = Character.GetGuitarMesh();
		const FTransform GuitarToSpine = GuitarMesh->GetRelativeTransform()
			* Character.GetMesh()->GetSocketTransform(GuitarMesh->GetAttachSocketName(), RTS_ParentBoneSpace);
		const FGuitarFrettingPose Targets = PluckingState.EvaluatePose(*PluckingDefinition,
			ReadFretboard(*GuitarMesh)).GetTransformed(GuitarToSpine);
		PluckingPose.Wrist = Targets.Wrist;
		PluckingPose.Elbow = Targets.Elbow;
		PluckingPose.Index = Targets.Index;
		PluckingPose.Middle = Targets.Middle;
		PluckingPose.Ring = Targets.Ring;
		PluckingPose.Little = Targets.Little;
		PluckingPose.Thumb = Targets.Thumb;
		PluckingPose.Alpha = 1.0f;
	}
}

void UGuitarAnimInstance::NotifyStringPlucked(int32 StringIndex)
{
	PluckingState.PluckString(StringIndex);
}

void UGuitarAnimInstance::ResetFrettingState()
{
	bGripPoseInitialized = false;
	CachedChordId = NAME_None;
	FrettingPose.Alpha = 0.0f;
}

void UGuitarAnimInstance::UpdateFrettingPose(const AGuitarCharacter& Character, float DeltaSeconds)
{
	const UStaticMeshComponent& GuitarMesh = *Character.GetGuitarMesh();
	const FGuitarChord* Chord = Character.GetGuitarInstrument()->GetCurrentChord();
	const FName ChordId = Chord ? Chord->ChordId : NAME_None;
	if (!bGripPoseInitialized || ChordId != CachedChordId)
	{
		UpdateChordTarget(Chord, GuitarMesh);
		CachedChordId = ChordId;
	}

	if (!bGripPoseInitialized)
	{
		SmoothedGripPose = FrettingDefinition->BaseGripPose;
		bGripPoseInitialized = true;
	}

	const float BlendAlpha = 1.0f - FMath::Exp(-3.0f * FMath::Max(DeltaSeconds, 0.0f)
		/ FMath::Max(FrettingDefinition->ChordTransitionSeconds, 0.01f));
	SmoothedGripPose.BlendTowards(TargetGripPose, BlendAlpha);
	GuitarFretting::ConstrainFingerBends(SmoothedGripPose, FrettingDefinition->FingerGeometry);

	// 用附着关系转换到 spine_003，避免读取上一帧求解后的手指造成反馈。
	const FTransform GuitarToSpine = GuitarMesh.GetRelativeTransform()
		* Character.GetMesh()->GetSocketTransform(GuitarMesh.GetAttachSocketName(), RTS_ParentBoneSpace);
	FrettingPose = SmoothedGripPose.GetTransformed(GuitarToSpine);
	FrettingPose.Alpha = 1.0f;
}

void UGuitarAnimInstance::UpdateChordTarget(const FGuitarChord* Chord, const UStaticMeshComponent& GuitarMesh)
{
	TargetGripPose = FrettingDefinition->BaseGripPose;
	if (!Chord)
	{
		return;
	}

	const FGuitarFretboard Board = ReadFretboard(GuitarMesh);
	GuitarFretting::SolveChord(*Chord, Board, FrettingDefinition->FingerGeometry,
		FrettingDefinition->OpenGripPose, FrettingDefinition->BarreGripPose, TargetGripPose);
}
