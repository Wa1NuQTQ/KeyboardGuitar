#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/Animation/Shared/GuitarFingerTypes.h"
#include "GuitarPluckingDefinition.generated.h"

// 吉他局部空间的右手准备姿势与五指尺寸；运行时由此生成拨弦目标。
UCLASS(BlueprintType)
class KEYBOARDGUITAR_API UGuitarPluckingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UGuitarPluckingDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Right Hand")
	FTransform Wrist = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Right Hand")
	FVector Elbow = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finger Targets")
	FGuitarFingerTarget Index;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finger Targets")
	FGuitarFingerTarget Middle;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finger Targets")
	FGuitarFingerTarget Ring;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finger Targets")
	FGuitarFingerTarget Little;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finger Targets")
	FGuitarFingerTarget Thumb;

	// 食指、中指、无名指、小指、拇指；用当前人物几何约束弯曲与骨长。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Finger Geometry")
	TArray<FGuitarFingerGeometry> FingerGeometry;
};
