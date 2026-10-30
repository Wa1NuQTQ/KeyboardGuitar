#include "GuitarDefinition.h"

UGuitarDefinition::UGuitarDefinition()
{
	// 默认吉他空弦音高为标准调弦
	OpenStringMidiNotes = {64, 59, 55, 50, 45, 40};
}

const FGuitarChord* UGuitarDefinition::FindChordById(FName ChordId) const
{
	return Chords.FindByPredicate(
		[ChordId](const FGuitarChord& Chord)
		{
			return Chord.ChordId == ChordId;
		});
}
