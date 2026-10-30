#include "GuitarTypes.h"

// 根据根音和和弦类型生成和弦标识
FName Guitar::MakeChordId(int32 RootPitchClass, EGuitarChordType Type)
{
	static const TCHAR* RootNames[] =
	{
		TEXT("C"), TEXT("C#"), TEXT("D"), TEXT("Eb"),
		TEXT("E"), TEXT("F"), TEXT("F#"), TEXT("G"),
		TEXT("Ab"), TEXT("A"), TEXT("Bb"), TEXT("B")
	};
	static const TCHAR* ChordSuffixes[] =
	{
		TEXT(""), TEXT("m"), TEXT("7"), TEXT("maj7"),
		TEXT("m7"), TEXT("sus2"), TEXT("sus4"), TEXT("add9")
	};

	const int32 PitchClass = (RootPitchClass % 12 + 12) % 12;
	const int32 TypeIndex = static_cast<int32>(Type);

	const TCHAR* RootName = RootNames[PitchClass];
	return FName(*(FString(RootName) + ChordSuffixes[TypeIndex]));
}
