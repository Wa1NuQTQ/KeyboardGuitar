#pragma once

#include "CoreMinimal.h"
#include "GuitarFingerTypes.generated.h"

// 动画目标，不包含音高规则。配置使用吉他局部空间，传给 CR 时使用 spine_003 局部空间。
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarFingerTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FTransform Tip = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector Bend = FVector::ZeroVector;

	// 骨骼末端与实际指腹接触点的差，单位 cm，沿吉他局部轴表示。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector ContactOffsetCm = FVector(0.0, 0.25, 0.0);
};

// 模型尺寸在编辑器中从基础握姿提取，运行时不读取上一帧已经求解的手指。
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarFingerGeometry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector RootInWrist = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector SegmentLengthsCm = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector TipAxis = FVector(0.0, -1.0, 0.0);
};
