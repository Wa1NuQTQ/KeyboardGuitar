#pragma once

#include "CoreMinimal.h"

namespace GuitarCarry
{
	// 所有变换与枢轴均为人物 Mesh 组件空间，BoundsCenter 为吉他网格局部中心。
	// Progress01：0 背琴，1 演奏；调用方平滑进度，本函数不保存历史状态。
	// 琴中心从躯干右侧（Mesh -X）绕行，避免前后位置直接插值穿过身体。
	KEYBOARDGUITAR_API FTransform CalculateTransform(float Progress01,
		const FTransform& FrontGuitarToMesh, const FTransform& CarryGuitarToMesh,
		const FVector& BoundsCenter, const FVector& TorsoPivot);
}
