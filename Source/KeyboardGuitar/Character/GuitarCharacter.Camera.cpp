#include "GuitarCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

void AGuitarCharacter::CreateCameraComponents()
{
	// 视角由 Controller 驱动，人物朝向由移动方向决定。
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 35.0f, 60.0f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 限制教室暗部触发的过度提亮，保留窗光下的皮肤与白衣细节。亮度范围单位为 EV100。
	FPostProcessSettings& ViewSettings = FollowCamera->PostProcessSettings;
	ViewSettings.bOverride_AutoExposureMethod = true;
	ViewSettings.AutoExposureMethod = AEM_Histogram;
	ViewSettings.bOverride_AutoExposureMinBrightness = true;
	ViewSettings.AutoExposureMinBrightness = -2.5f;
	ViewSettings.bOverride_AutoExposureMaxBrightness = true;
	ViewSettings.AutoExposureMaxBrightness = 2.0f;
	ViewSettings.bOverride_AutoExposureBias = true;
	ViewSettings.AutoExposureBias = 0.0f;
	ViewSettings.bOverride_BloomIntensity = true;
	ViewSettings.BloomIntensity = 0.2f;
	ViewSettings.bOverride_LumenSkylightLeaking = true;
	ViewSettings.LumenSkylightLeaking = 0.1f;
}

bool AGuitarCharacter::IsGuitarCameraTransitioning() const
{
	return bMovementCameraSaved && CameraBlendAlpha != TargetGuitarTransitionProgress;
}

void AGuitarCharacter::SaveMovementCamera()
{
	MovementCamera.ViewRotation = CameraBoom->GetTargetRotation();
	MovementCamera.BoomRelativeRotation = CameraBoom->GetRelativeRotation();
	MovementCamera.CameraRelativeTransform = FollowCamera->GetRelativeTransform();
	MovementCamera.SocketOffset = CameraBoom->SocketOffset;
	MovementCamera.TargetOffset = CameraBoom->TargetOffset;
	MovementCamera.ArmLength = CameraBoom->TargetArmLength;
	MovementCamera.FieldOfView = FollowCamera->FieldOfView;
	MovementCamera.bUsePawnControlRotation = CameraBoom->bUsePawnControlRotation;
	MovementCamera.bCameraUsesControlRotation = FollowCamera->bUsePawnControlRotation;
	MovementCamera.bInheritPitch = CameraBoom->bInheritPitch;
	MovementCamera.bInheritYaw = CameraBoom->bInheritYaw;
	MovementCamera.bInheritRoll = CameraBoom->bInheritRoll;
	MovementCamera.bEnableCameraLag = CameraBoom->bEnableCameraLag;
	MovementCamera.bEnableCameraRotationLag = CameraBoom->bEnableCameraRotationLag;
	bMovementCameraSaved = true;

	// 临时直接驱动弹簧臂，保留 Controller 的原视角。碰撞检测继续由弹簧臂处理。
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
	FollowCamera->bUsePawnControlRotation = false;
	CameraBoom->SetWorldRotation(MovementCamera.ViewRotation);
	GuitarCameraWorldRotation = FRotator(GuitarCameraRotation.Pitch,
		GetActorRotation().Yaw + GuitarCameraRotation.Yaw, GuitarCameraRotation.Roll).Quaternion();
}

void AGuitarCharacter::UpdateGuitarCamera(float DeltaSeconds)
{
	if (!bMovementCameraSaved)
	{
		return;
	}
	const bool bWasReady = CameraBlendAlpha == 1.0f;
	CameraBlendAlpha = FMath::FInterpConstantTo(CameraBlendAlpha, TargetGuitarTransitionProgress,
		FMath::Max(DeltaSeconds, 0.0f), 1.0f / FMath::Max(CameraTransitionSeconds, 0.1f));
	const float Blend = FMath::SmoothStep(0.0f, 1.0f, CameraBlendAlpha);
	CameraBoom->SetWorldRotation(FQuat::Slerp(MovementCamera.ViewRotation.Quaternion(), GuitarCameraWorldRotation, Blend));
	CameraBoom->TargetArmLength = FMath::Lerp(MovementCamera.ArmLength, GuitarCameraArmLength, Blend);
	CameraBoom->SocketOffset = FMath::Lerp(MovementCamera.SocketOffset, FVector::ZeroVector, Blend);
	CameraBoom->TargetOffset = FMath::Lerp(MovementCamera.TargetOffset, GuitarCameraTargetOffset, Blend);
	FollowCamera->SetFieldOfView(FMath::Lerp(MovementCamera.FieldOfView, GuitarCameraFieldOfView, Blend));
	FTransform CameraRelative;
	CameraRelative.Blend(MovementCamera.CameraRelativeTransform, FTransform::Identity, Blend);
	FollowCamera->SetRelativeTransform(CameraRelative);

	if (CameraBlendAlpha == 0.0f && TargetGuitarTransitionProgress == 0.0f)
	{
		CameraBoom->SetRelativeRotation(MovementCamera.BoomRelativeRotation);
		CameraBoom->bUsePawnControlRotation = MovementCamera.bUsePawnControlRotation;
		CameraBoom->bInheritPitch = MovementCamera.bInheritPitch;
		CameraBoom->bInheritYaw = MovementCamera.bInheritYaw;
		CameraBoom->bInheritRoll = MovementCamera.bInheritRoll;
		CameraBoom->bEnableCameraLag = MovementCamera.bEnableCameraLag;
		CameraBoom->bEnableCameraRotationLag = MovementCamera.bEnableCameraRotationLag;
		FollowCamera->bUsePawnControlRotation = MovementCamera.bCameraUsesControlRotation;
		bMovementCameraSaved = false;
	}
	else if (!bWasReady && CameraBlendAlpha == 1.0f)
	{
		OnGuitarCameraReady.Broadcast();
	}
}
