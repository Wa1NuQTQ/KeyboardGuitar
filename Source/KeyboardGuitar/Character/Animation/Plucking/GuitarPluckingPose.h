#pragma once

#include "CoreMinimal.h"
#include "Character/Animation/Fretting/GuitarFrettingPose.h"
#include "GuitarPluckingPose.generated.h"

class UGuitarPluckingDefinition;

struct FGuitarFretboard;

// 右手五指目标与四指拨弦进度；小指保持放松，不参与拨弦计时。
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarPluckingPose
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThumbCurl01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IndexCurl01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MiddleCurl01 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RingCurl01 = 0.0f;

	// 拇指的连续零基弦位置：3/4/5 对应第四/五/六弦；准备时悬在第六弦。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking", meta = (ClampMin = "3.0", ClampMax = "5.0"))
	float ThumbStringPosition = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FGuitarFingerTarget Index;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FGuitarFingerTarget Middle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FGuitarFingerTarget Ring;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FGuitarFingerTarget Little;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FGuitarFingerTarget Thumb;

	// 手腕和肘部目标使用 spine_003 局部空间，和吉他一起移动。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FTransform Wrist = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	FVector Elbow = FVector::ZeroVector;

	// 直接预览 CR 时为 0，保留手动控制；动画实例提供目标后为 1。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plucking")
	float Alpha = 0.0f;
};

// 游戏线程上的短动作状态；不访问输入、音频或骨骼，由动画实例驱动。
class KEYBOARDGUITAR_API FGuitarPluckingState
{

public:
	// StringIndex 为 0..5；前三根弦分别使用无名指、中指、食指，其余使用拇指。
	void PluckString(int32 StringIndex);
	void Update(float DeltaSeconds);
	void Reset();
	float GetReturnLift(int32 FingerIndex) const;
	// 根据准备姿势和本次拨弦进度生成吉他局部目标，不读取场景对象。
	FGuitarFrettingPose EvaluatePose(const UGuitarPluckingDefinition& Definition,
		const FGuitarFretboard& Board) const;

	const FGuitarPluckingPose& GetPose() const { return Pose; }

private:
	struct FFingerStroke
	{
		float ElapsedSeconds = 0.0f;
		float StartCurl01 = 0.0f;
		float ReturnLift01 = 0.0f;
		float StartLift01 = 0.0f;
		bool bActive = false;
	};

	FGuitarPluckingPose Pose;
	// 顺序：拇指、食指、中指、无名指；每根手指可独立完成动作。
	FFingerStroke Strokes[4];
	float TargetThumbStringPosition = 5.0f;
};
