#include "GuitarInstrumentComponent.h"

#include "GuitarDefinition.h"

UGuitarInstrumentComponent::UGuitarInstrumentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGuitarInstrumentComponent::SelectChord(FName ChordId)
{
	CurrentChordId = ChordId;
}

void UGuitarInstrumentComponent::ClearChord()
{
	CurrentChordId = NAME_None;
}

const FGuitarChord* UGuitarInstrumentComponent::GetCurrentChord() const
{
	return GuitarDefinition->FindChordById(CurrentChordId);
}

void UGuitarInstrumentComponent::PluckString(int32 StringIndex)
{
	if (StringIndex < 0 || StringIndex >= Guitar::StringCount) return;
	FGuitarPlayedNote Note;
	Note.InputTimeSeconds = FPlatformTime::Seconds();
	Note.ChordId = CurrentChordId;
	Note.StringIndex = StringIndex;
	Note.MidiNote = GuitarDefinition->OpenStringMidiNotes[StringIndex];
	// 未选和弦时弹空弦；选中和弦后应用对应指法
	if (const FGuitarChord* Chord = GetCurrentChord())
	{
		const FGuitarStringFingering& Fingering = Chord->StringFingerings[StringIndex];
		Note.bMuted = Fingering.bMuted;
		Note.Fret = Fingering.Fret;
		Note.MidiNote += Note.Fret;
	}
	OnStringPlucked.Broadcast(StringIndex);
	if (!Note.bMuted) OnNotePlayed.Broadcast(StringIndex, Note.MidiNote);
	OnPluckResolved.Broadcast(Note);
}
