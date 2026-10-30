#include "GuitarSong.h"
#include "Instrument/GuitarDefinition.h"

FName UGuitarSong::GetChordAt(double TimeSeconds) const
{
	FName Chord = NAME_None;
	for (const FGuitarChordChangeEvent& Change : ChordChanges)
	{
		if (Change.StartTimeSeconds > TimeSeconds) break;
		Chord = Change.ChordId;
	}
	return Chord;
}

bool UGuitarSong::ValidateChart(const UGuitarDefinition& Definition, FString& Error) const
{
	auto Fail = [&Error](const TCHAR* Message) { Error = Message; return false; };
	if (!FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0 || BeatsPerMinute < 20 || BeatsPerMinute > 240 || Plucks.IsEmpty())
		return Fail(TEXT("曲谱时长、速度或音符数量无效"));
	double Previous = -1.0;
	for (const FGuitarChordChangeEvent& Change : ChordChanges)
	{
		if (!FMath::IsFinite(Change.StartTimeSeconds) || Change.StartTimeSeconds < 0 || Change.StartTimeSeconds <= Previous || Change.StartTimeSeconds >= DurationSeconds)
			return Fail(TEXT("和弦时间必须递增，并处于歌曲范围内"));
		if (!Change.ChordId.IsNone() && !Definition.FindChordById(Change.ChordId))
			return Fail(TEXT("曲谱引用了未定义的和弦"));
		Previous = Change.StartTimeSeconds;
	}
	Previous = -1.0;
	TSet<int32> StringsAtTime;
	for (const FGuitarPluckEvent& Pluck : Plucks)
	{
		if (!FMath::IsFinite(Pluck.StartTimeSeconds) || Pluck.StartTimeSeconds < 0 || Pluck.StartTimeSeconds < Previous || Pluck.StartTimeSeconds >= DurationSeconds || Pluck.StringIndex < 0 || Pluck.StringIndex >= Guitar::StringCount)
			return Fail(TEXT("拨弦时间、顺序或弦号无效"));
		if (Pluck.StartTimeSeconds != Previous) StringsAtTime.Reset();
		if (StringsAtTime.Contains(Pluck.StringIndex)) return Fail(TEXT("同一时刻同一根弦不能重复"));
		StringsAtTime.Add(Pluck.StringIndex);
		const FGuitarChord* Chord = Definition.FindChordById(GetChordAt(Pluck.StartTimeSeconds));
		if (Chord && (Chord->StringFingerings.Num() != Guitar::StringCount || Chord->StringFingerings[Pluck.StringIndex].bMuted))
			return Fail(TEXT("拨弦目标为制音弦或指法不完整"));
		Previous = Pluck.StartTimeSeconds;
	}
	Error.Reset();
	return true;
}
