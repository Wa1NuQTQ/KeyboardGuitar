#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GuitarCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UGuitarInstrumentComponent;
class UGuitarAudioComponent;

// 玩家角色
UCLASS()
class KEYBOARDGUITAR_API AGuitarCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGuitarCharacter();

	// 获取人物持有的吉他网格和乐器组件
	UStaticMeshComponent* GetGuitarMesh() const { return GuitarMesh; }
	UGuitarInstrumentComponent* GetGuitarInstrument() const { return GuitarInstrument; }
	// 获取演奏姿势的混合权重，0 为行走姿势，1 为持琴姿势
	float GetGuitarPoseAlpha() const { return GuitarPoseAlpha; }
	// 切换背琴、持琴与相机的呈现目标
	void SetGuitarModePresentation(bool bPlayingGuitar);
	// 查询镜头是否正在过渡，以及是否已到演奏位置
	bool IsGuitarCameraTransitioning() const;
	bool IsGuitarCameraReady() const { return CameraBlendAlpha == 1.0f && TargetGuitarTransitionProgress == 1.0f; }
	// 演奏镜头到位后通知控制器显示选歌页
	FSimpleMulticastDelegate OnGuitarCameraReady;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// 相机组件创建与过渡逻辑在 GuitarCharacter.Camera.cpp
	void CreateCameraComponents();
	void SaveMovementCamera();
	void UpdateGuitarCamera(float DeltaSeconds);

	// 更新吉他收放位置，并把拨弦事件转交动画实例
	void UpdateGuitarPresentation();
	void HandleStringPlucked(int32 StringIndex);

	// 演奏镜头到人物的距离，单位 cm
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guitar", meta = (ClampMin = "50", Units = "cm"))
	float GuitarCameraArmLength = 175.0f;

	// 演奏镜头的注视中心偏移，单位 cm
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guitar", meta = (Units = "cm"))
	FVector GuitarCameraTargetOffset = FVector(0.0f, 0.0f, 30.0f);

	// 相对人物的观看方向；Yaw 180 度时，相机位于人物正前方。
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guitar")
	FRotator GuitarCameraRotation = FRotator(-6.0f, 190.0f, 0.0f);

	// 演奏镜头的视野角，单位度
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guitar", meta = (ClampMin = "30", ClampMax = "110", Units = "deg"))
	float GuitarCameraFieldOfView = 65.0f;

	// 移动与演奏镜头切换的时间，单位秒
	UPROPERTY(EditDefaultsOnly, Category = "Camera|Guitar", meta = (ClampMin = "0.1", Units = "s"))
	float CameraTransitionSeconds = 0.65f;

	// 进入演奏前缓存实际相机设置，退出时原样恢复
	struct FMovementCameraState
	{
		FRotator ViewRotation;
		FRotator BoomRelativeRotation;
		FTransform CameraRelativeTransform;
		FVector SocketOffset;
		FVector TargetOffset;
		float ArmLength;
		float FieldOfView;
		bool bUsePawnControlRotation;
		bool bCameraUsesControlRotation;
		bool bInheritPitch;
		bool bInheritYaw;
		bool bInheritRoll;
		bool bEnableCameraLag;
		bool bEnableCameraRotationLag;
	};

	FMovementCameraState MovementCamera;
	FQuat GuitarCameraWorldRotation = FQuat::Identity;
	float CameraBlendAlpha = 0.0f;
	bool bMovementCameraSaved = false;

	// 吉他到 spine_003 的局部变换；沿用已有插槽，运行时只改变组件相对变换。
	UPROPERTY(EditDefaultsOnly, Category = "Guitar|Presentation")
	FTransform CarriedGuitarTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, Category = "Guitar|Presentation", meta = (ClampMin = "0.1", Units = "s"))
	float GuitarTransitionSeconds = 0.55f;

	// 动画蓝图读取的持琴姿势权重
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guitar|Presentation", meta = (AllowPrivateAccess = "true"))
	float GuitarPoseAlpha = 0.0f;

	float GuitarTransitionProgress = 0.0f;
	float TargetGuitarTransitionProgress = 0.0f;
	FTransform HeldGuitarRelativeTransform;
	FVector GuitarBoundsCenter = FVector::ZeroVector;

	// 相机弹簧臂与跟随相机
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// 人物持有的吉他网格
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GuitarMesh;

	// 乐器规则与声音播放组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGuitarInstrumentComponent> GuitarInstrument;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Guitar", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGuitarAudioComponent> GuitarAudio;
};
