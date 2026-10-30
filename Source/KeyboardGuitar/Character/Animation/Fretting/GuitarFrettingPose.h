#pragma once

#include "CoreMinimal.h"
#include "Character/Animation/Shared/GuitarFingerTypes.h"
#include "GuitarFrettingPose.generated.h"

USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarFrettingPose
{
	GENERATED_BODY()

	// 只插值运动目标；ContactOffsetCm 是校准数据，Alpha 由动画实例设置。
	void BlendTowards(const FGuitarFrettingPose& Target, float BlendAlpha);

	// 将运动目标从当前局部空间转换到目标空间，不读取场景对象。
	FGuitarFrettingPose GetTransformed(const FTransform& LocalToTarget) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FTransform Wrist = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FVector Elbow = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FGuitarFingerTarget Index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FGuitarFingerTarget Middle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FGuitarFingerTarget Ring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FGuitarFingerTarget Little;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	FGuitarFingerTarget Thumb;

	// 编辑器直接预览 CR 时为 0，保留手动控制；人物动画实例准备好目标后为 1。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fretting")
	float Alpha = 0.0f;
};
