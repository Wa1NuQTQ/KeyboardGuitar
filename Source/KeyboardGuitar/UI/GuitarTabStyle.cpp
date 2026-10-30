#include "GuitarTabStyle.h"
#include "Instrument/GuitarTypes.h"

const FGuitarTabColors& GuitarUI::GetTabColors(EGuitarChordType Type)
{
	// 与 EGuitarChordType 顺序一致：米金、雾蓝、陶橘、玫粉、淡紫、青绿、橄榄、丁香。
	static const FGuitarTabColors Colors[] =
	{
		{FLinearColor(FColor(245, 233, 206, 242)), FLinearColor(FColor(115, 88, 32))},
		{FLinearColor(FColor(228, 237, 248, 242)), FLinearColor(FColor(54, 94, 136))},
		{FLinearColor(FColor(244, 225, 213, 242)), FLinearColor(FColor(151, 83, 47))},
		{FLinearColor(FColor(244, 226, 232, 242)), FLinearColor(FColor(136, 77, 102))},
		{FLinearColor(FColor(233, 227, 244, 242)), FLinearColor(FColor(98, 81, 140))},
		{FLinearColor(FColor(220, 240, 233, 242)), FLinearColor(FColor(36, 105, 91))},
		{FLinearColor(FColor(236, 236, 207, 242)), FLinearColor(FColor(92, 105, 38))},
		{FLinearColor(FColor(240, 225, 242, 242)), FLinearColor(FColor(132, 85, 133))}
	};
	return Colors[static_cast<int32>(Type)];
}

const FGuitarChordStyle* GuitarUI::FindChordStyle(FName ChordId)
{
	static const TMap<FName, FGuitarChordStyle> Styles = []
	{
		const FColor RootColors[] =
		{
			{42, 101, 84}, {124, 92, 44}, {152, 78, 39}, {117, 73, 116},
			{47, 91, 142}, {96, 78, 146}, {87, 109, 44}, {151, 63, 87},
			{99, 90, 129}, {36, 105, 120}, {128, 94, 36}, {113, 79, 57}
		};
		TMap<FName, FGuitarChordStyle> Result;
		for (int32 Root = 0; Root < 12; ++Root)
			for (int32 Type = 0; Type < 8; ++Type)
			{
				const auto ChordType = static_cast<EGuitarChordType>(Type);
				Result.Add(Guitar::MakeChordId(Root, ChordType), FGuitarChordStyle{ChordType, FLinearColor(RootColors[Root])});
			}
		return Result;
	}();
	return Styles.Find(ChordId);
}

FLinearColor GuitarUI::GetChordColor(FName ChordId)
{
	const FGuitarChordStyle* Style = FindChordStyle(ChordId);
	return Style ? Style->Ink : FLinearColor(0.22f, 0.27f, 0.19f);
}
