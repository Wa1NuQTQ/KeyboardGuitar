#pragma once

#include "CoreMinimal.h"

// 左右手共用的四个弦端定位点，均为吉他网格局部厘米；不读取场景或骨骼。
struct KEYBOARDGUITAR_API FGuitarFretboard
{
	FVector Nut1;
	FVector Nut6;
	FVector Bridge1;
	FVector Bridge6;

	FVector PressPosition(int32 StringIndex, int32 Fret) const;
};
