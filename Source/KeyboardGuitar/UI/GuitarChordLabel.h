#pragma once

#include "CoreMinimal.h"

enum class EGuitarChordType : uint8;

namespace GuitarUI
{
	// 和弦名不附按键，类型切换提醒另行显示。
	KEYBOARDGUITAR_API FString MakeChordLabel(FName ChordId);
	// 仅在待用类型不符合下一目标时提醒，切换正确后隐藏。
	KEYBOARDGUITAR_API FString MakeTypeSwitchHint(FName ChordId, EGuitarChordType SelectedType);
}
