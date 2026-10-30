#pragma once

#include "CoreMinimal.h"

enum class EGuitarChordType : uint8;

struct FGuitarTabColors
{
	FLinearColor Paper;
	FLinearColor Ink;
};

struct FGuitarChordStyle
{
	EGuitarChordType Type;
	FLinearColor Ink;
};

namespace GuitarUI
{
	// 谱纸、音符和设置中的颜色对照共用同一组配色。
	KEYBOARDGUITAR_API const FGuitarTabColors& GetTabColors(EGuitarChordType Type);
	// 和弦名按根音分色，与已选类型控制的谱纸颜色分开。
	KEYBOARDGUITAR_API const FGuitarChordStyle* FindChordStyle(FName ChordId);
	KEYBOARDGUITAR_API FLinearColor GetChordColor(FName ChordId);
}
