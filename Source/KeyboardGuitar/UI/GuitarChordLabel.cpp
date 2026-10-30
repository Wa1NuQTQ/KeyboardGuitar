#include "GuitarChordLabel.h"
#include "GuitarTabStyle.h"

FString GuitarUI::MakeChordLabel(FName ChordId)
{
	return ChordId.IsNone() ? TEXT("空弦") : ChordId.ToString();
}

FString GuitarUI::MakeTypeSwitchHint(FName ChordId, EGuitarChordType SelectedType)
{
	const FGuitarChordStyle* Style = FindChordStyle(ChordId);
	if (!Style || Style->Type == SelectedType) return TEXT("");
	const TCHAR* Names[] = {TEXT("大三"), TEXT("小三"), TEXT("属七"), TEXT("大七"), TEXT("小七"), TEXT("sus2"), TEXT("sus4"), TEXT("add9")};
	const TCHAR* Keys[] = {TEXT("Q"), TEXT("W"), TEXT("R"), TEXT("T"), TEXT("Y"), TEXT("U"), TEXT("I"), TEXT("O")};
	const int32 TypeIndex = static_cast<int32>(Style->Type);
	return FString::Printf(TEXT("切换%s：%s"), Names[TypeIndex], Keys[TypeIndex]);
}
