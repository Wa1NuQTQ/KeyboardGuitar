#include "GuitarFretboard.h"

#include "Instrument/GuitarTypes.h"

FVector FGuitarFretboard::PressPosition(int32 StringIndex, int32 Fret) const
{
	const double StringAlpha = static_cast<double>(StringIndex) / (Guitar::StringCount - 1);
	const FVector Nut = FMath::Lerp(Nut1, Nut6, StringAlpha);
	const FVector Bridge = FMath::Lerp(Bridge1, Bridge6, StringAlpha);
	const double PreviousFretAlpha = 1.0 - FMath::Pow(2.0, -(Fret - 1) / 12.0);
	const double NextFretAlpha = 1.0 - FMath::Pow(2.0, -Fret / 12.0);
	return FMath::Lerp(Nut, Bridge, FMath::Lerp(PreviousFretAlpha, NextFretAlpha, 0.8));
}
