#include "GuitarCharacter.h"

#include "Audio/GuitarAudioComponent.h"
#include "Character/Animation/GuitarAnimInstance.h"
#include "Character/Animation/GuitarCarryMotion.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Instrument/GuitarInstrumentComponent.h"

AGuitarCharacter::AGuitarCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CreateCameraComponents();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 度/秒
	GetCharacterMovement()->MaxWalkSpeed = 200.0f; // cm/s；实际角色可在蓝图默认值中覆盖。

	// 吉他跟随躯干插槽，位置和比例由骨架插槽及蓝图资源配置。
	GuitarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuitarMesh"));
	GuitarMesh->SetupAttachment(GetMesh(), TEXT("GuitarSocket"));
	GuitarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GuitarInstrument = CreateDefaultSubobject<UGuitarInstrumentComponent>(TEXT("GuitarInstrument"));
	GuitarAudio = CreateDefaultSubobject<UGuitarAudioComponent>(TEXT("GuitarAudio"));
}

void AGuitarCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 连接乐器的发声事件和拨弦动画事件
	GuitarInstrument->OnNotePlayed.AddUObject(GuitarAudio, &UGuitarAudioComponent::PlayNote);
	GuitarInstrument->OnStringPlucked.AddUObject(this, &AGuitarCharacter::HandleStringPlucked);
	HeldGuitarRelativeTransform = GuitarMesh->GetRelativeTransform();
	GuitarBoundsCenter = GuitarMesh->GetStaticMesh()->GetBoundingBox().GetCenter();
	GuitarTransitionProgress = TargetGuitarTransitionProgress;
	// 先更新琴的位置，再让动画追踪手部目标
	GetMesh()->AddTickPrerequisiteActor(this);
	UpdateGuitarPresentation();
	SetActorTickEnabled(IsGuitarCameraTransitioning());
}

void AGuitarCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GuitarInstrument->OnNotePlayed.RemoveAll(GuitarAudio);
	GuitarInstrument->OnStringPlucked.RemoveAll(this);

	Super::EndPlay(EndPlayReason);
}

void AGuitarCharacter::SetGuitarModePresentation(bool bPlayingGuitar)
{
	if (bPlayingGuitar && !bMovementCameraSaved)
	{
		SaveMovementCamera();
	}
	TargetGuitarTransitionProgress = bPlayingGuitar ? 1.0f : 0.0f;
	// 同一帧进入再退出时，尚未启动 Tick，也要恢复临时接管的相机设置。
	if (!bPlayingGuitar && bMovementCameraSaved && CameraBlendAlpha == 0.0f)
	{
		UpdateGuitarCamera(0.0f);
	}
	if (HasActorBegunPlay())
	{
		SetActorTickEnabled(GuitarTransitionProgress != TargetGuitarTransitionProgress || IsGuitarCameraTransitioning());
	}
}

void AGuitarCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// 收放吉他与切换镜头共用一次过渡更新，完成后关闭 Tick
	GuitarTransitionProgress = FMath::FInterpConstantTo(GuitarTransitionProgress,
		TargetGuitarTransitionProgress, FMath::Max(DeltaSeconds, 0.0f), 1.0f / FMath::Max(GuitarTransitionSeconds, 0.1f));
	UpdateGuitarPresentation();
	UpdateGuitarCamera(DeltaSeconds);
	if (GuitarTransitionProgress == TargetGuitarTransitionProgress && !IsGuitarCameraTransitioning())
	{
		SetActorTickEnabled(false);
	}
}

void AGuitarCharacter::UpdateGuitarPresentation()
{
	const float PathProgress = FMath::SmoothStep(0.0f, 1.0f, GuitarTransitionProgress);
	// 琴接近前胸后才抬起双臂；收琴时先放松双臂，再绕身到背后。
	GuitarPoseAlpha = FMath::SmoothStep(0.72f, 1.0f, PathProgress);
	const FTransform SocketToMesh = GetMesh()->GetSocketTransform(GuitarMesh->GetAttachSocketName(), RTS_Component);
	const FTransform FrontToMesh = HeldGuitarRelativeTransform * SocketToMesh;
	const FTransform CarryToMesh = CarriedGuitarTransform * GetMesh()->GetSocketTransform(TEXT("spine_003"), RTS_Component);
	const FVector TorsoPivot = GetMesh()->GetSocketTransform(TEXT("spine_001"), RTS_Component).GetLocation();
	const FTransform GuitarToMesh = GuitarCarry::CalculateTransform(PathProgress, FrontToMesh, CarryToMesh, GuitarBoundsCenter, TorsoPivot);
	GuitarMesh->SetRelativeTransform(GuitarToMesh.GetRelativeTransform(SocketToMesh));
}

void AGuitarCharacter::HandleStringPlucked(int32 StringIndex)
{
	// 始终取当前实例，Anim Class 重建后不会继续调用旧实例。
	if (UGuitarAnimInstance* Animation = Cast<UGuitarAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		Animation->NotifyStringPlucked(StringIndex);
	}
}
