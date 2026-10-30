#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GuitarFrettingPose.h"
#include "GuitarFrettingDefinition.generated.h"

// 人物的按弦校准数据。资产保存吉他局部空间的握姿；不保存当前和弦或求解状态。
UCLASS(BlueprintType)
class KEYBOARDGUITAR_API UGuitarFrettingDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UGuitarFrettingDefinition();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grip Templates")
	FGuitarFrettingPose BaseGripPose;

	// 开放和弦与横按和弦的搜索起点，不是每个和弦的最终姿势。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grip Templates")
	FGuitarFrettingPose OpenGripPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grip Templates")
	FGuitarFrettingPose BarreGripPose;

	// 固定顺序：食指、中指、无名指、小指、拇指。更换人物比例后重新校准。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Finger Geometry")
	TArray<FGuitarFingerGeometry> FingerGeometry;

	// 指数过渡在此时间内完成约 95%，单位秒。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.01", Units = "s"))
	float ChordTransitionSeconds = 0.2f;
};
