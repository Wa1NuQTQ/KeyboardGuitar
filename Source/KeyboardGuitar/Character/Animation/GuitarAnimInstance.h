#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/Animation/Fretting/GuitarFrettingPose.h"
#include "Character/Animation/Plucking/GuitarPluckingPose.h"
#include "GuitarAnimInstance.generated.h"

class AGuitarCharacter;
class UGuitarFrettingDefinition;
class UGuitarPluckingDefinition;
class UStaticMeshComponent;
struct FGuitarChord;

// 场景到动画的适配层：读取人物状态，生成供 ABP/CR 消费的快照。
UCLASS()
class KEYBOARDGUITAR_API UGuitarAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void NotifyStringPlucked(int32 StringIndex);

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 水平速度，单位 cm/s
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Locomotion")
	float GroundSpeed = 0.0f;

	// CR 接收的是 spine_003 局部目标，求值线程只消费此快照。
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Guitar|Runtime")
	FGuitarFrettingPose FrettingPose;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Guitar|Runtime")
	FGuitarPluckingPose PluckingPose;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Guitar|Runtime")
	float GuitarPoseAlpha = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Guitar|Configuration")
	TObjectPtr<UGuitarFrettingDefinition> FrettingDefinition;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Guitar|Configuration")
	TObjectPtr<UGuitarPluckingDefinition> PluckingDefinition;

private:
	void ResetFrettingState();
	void UpdateFrettingPose(const AGuitarCharacter& Character, float DeltaSeconds);
	void UpdatePluckingPose(const AGuitarCharacter& Character, float DeltaSeconds);
	void UpdateChordTarget(const FGuitarChord* Chord, const UStaticMeshComponent& GuitarMesh);

	// 全部缓存保持在吉他局部空间，人物转身不会拖动到旧的世界位置。
	FGuitarFrettingPose SmoothedGripPose;
	FGuitarFrettingPose TargetGripPose;
	FName CachedChordId = NAME_None;
	bool bGripPoseInitialized = false;
	FGuitarPluckingState PluckingState;
};
