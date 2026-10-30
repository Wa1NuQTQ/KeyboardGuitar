#include "GuitarPracticeSessionComponent.h"
#include "GuitarSong.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "Kismet/GameplayStatics.h"

namespace { const FString SaveSlot = TEXT("GuitarPractice"); }

UGuitarPracticeSessionComponent::UGuitarPracticeSessionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGuitarPracticeSessionComponent::BeginPlay()
{
	Super::BeginPlay();
	SavedRecords = Cast<UGuitarPracticeSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
	if (!SavedRecords) SavedRecords = NewObject<UGuitarPracticeSave>(this);
}

bool UGuitarPracticeSessionComponent::StartSong(UGuitarSong* Song, const UGuitarDefinition& Definition)
{
	if (!Song) { Error = TEXT("没有可演奏的曲目"); return false; }
	if (!Song->ValidateChart(Definition, Error)) return false;
	CurrentSong = Song;
	Result = {};
	Judgements.Init(EGuitarJudgement::Pending, Song->Plucks.Num());
	NextExpiredEvent = 0;
	LastJudgement = EGuitarJudgement::Pending;
	FeedbackTimeSeconds = -100.0;
	SongStartSeconds = FPlatformTime::Seconds() + CountdownSeconds;
	SetStage(EGuitarPracticeStage::Countdown);
	return true;
}

void UGuitarPracticeSessionComponent::StartFreePlay()
{
	CurrentSong = nullptr;
	Result = {};
	LastJudgement = EGuitarJudgement::Pending;
	SongStartSeconds = FPlatformTime::Seconds();
	SetStage(EGuitarPracticeStage::FreePlay);
}

void UGuitarPracticeSessionComponent::SelectSongs()
{
	CurrentSong = nullptr;
	Judgements.Reset();
	SetStage(EGuitarPracticeStage::Selecting);
}

void UGuitarPracticeSessionComponent::SetStage(EGuitarPracticeStage NewStage)
{
	Stage = NewStage;
	SetComponentTickEnabled(Stage == EGuitarPracticeStage::Countdown || Stage == EGuitarPracticeStage::Playing);
	OnStageChanged.Broadcast();
}

double UGuitarPracticeSessionComponent::GetSongTimeSeconds() const
{
	if (Stage == EGuitarPracticeStage::Paused) return PausedSongTimeSeconds;
	if (Stage == EGuitarPracticeStage::Results && CurrentSong) return CurrentSong->DurationSeconds;
	return FPlatformTime::Seconds() - SongStartSeconds;
}

void UGuitarPracticeSessionComponent::Pause()
{
	if (Stage != EGuitarPracticeStage::Countdown && !CanPlay()) return;
	PausedSongTimeSeconds = GetSongTimeSeconds();
	StageBeforePause = Stage;
	SetStage(EGuitarPracticeStage::Paused);
}

void UGuitarPracticeSessionComponent::Resume()
{
	if (Stage != EGuitarPracticeStage::Paused) return;
	SongStartSeconds = FPlatformTime::Seconds() - PausedSongTimeSeconds;
	SetStage(StageBeforePause);
}

void UGuitarPracticeSessionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshClock();
}

void UGuitarPracticeSessionComponent::RefreshClock()
{
	const double Time = GetSongTimeSeconds();
	if (Stage == EGuitarPracticeStage::Countdown && Time >= 0.0) SetStage(EGuitarPracticeStage::Playing);
	if (Stage != EGuitarPracticeStage::Playing) return;
	ExpireNotes(Time);
	if (Time >= CurrentSong->DurationSeconds + GoodWindowSeconds)
	{
		SaveRecord();
		SetStage(EGuitarPracticeStage::Results);
	}
}

void UGuitarPracticeSessionComponent::JudgePluck(const FGuitarPlayedNote& Note)
{
	if (Stage != EGuitarPracticeStage::Playing) return;
	const double Time = Note.InputTimeSeconds - SongStartSeconds;
	// 判定使用输入快照的时间，不使用声音和委托回调处理完毕的时间
	ExpireNotes(Time);
	int32 Candidate = INDEX_NONE;
	double Nearest = GoodWindowSeconds + UE_DOUBLE_SMALL_NUMBER;
	for (int32 Index = NextExpiredEvent; Index < CurrentSong->Plucks.Num(); ++Index)
	{
		const FGuitarPluckEvent& Event = CurrentSong->Plucks[Index];
		if (Event.StartTimeSeconds > Time + GoodWindowSeconds) break;
		const double Distance = FMath::Abs(Event.StartTimeSeconds - Time);
		if (Event.StringIndex == Note.StringIndex && Judgements[Index] == EGuitarJudgement::Pending && Distance < Nearest)
		{
			Candidate = Index;
			Nearest = Distance;
		}
	}
	EGuitarJudgement Judgement = EGuitarJudgement::Wrong;
	if (Candidate != INDEX_NONE && !Note.bMuted && Note.ChordId == CurrentSong->GetChordAt(CurrentSong->Plucks[Candidate].StartTimeSeconds))
		Judgement = Nearest <= PerfectWindowSeconds + UE_DOUBLE_SMALL_NUMBER ? EGuitarJudgement::Perfect : EGuitarJudgement::Good;
	ApplyJudgement(Candidate, Judgement);
}

void UGuitarPracticeSessionComponent::ExpireNotes(double TimeSeconds)
{
	while (NextExpiredEvent < CurrentSong->Plucks.Num() && CurrentSong->Plucks[NextExpiredEvent].StartTimeSeconds + GoodWindowSeconds + UE_DOUBLE_SMALL_NUMBER < TimeSeconds)
	{
		if (Judgements[NextExpiredEvent] == EGuitarJudgement::Pending) ApplyJudgement(NextExpiredEvent, EGuitarJudgement::Miss);
		++NextExpiredEvent;
	}
}

void UGuitarPracticeSessionComponent::ApplyJudgement(int32 EventIndex, EGuitarJudgement Judgement)
{
	if (EventIndex != INDEX_NONE) Judgements[EventIndex] = Judgement;
	LastJudgement = Judgement;
	FeedbackTimeSeconds = FPlatformTime::Seconds();
	if (Judgement == EGuitarJudgement::Perfect || Judgement == EGuitarJudgement::Good)
	{
		Result.Score += Judgement == EGuitarJudgement::Perfect ? 1000 : 650;
		Judgement == EGuitarJudgement::Perfect ? ++Result.Perfect : ++Result.Good;
		++Result.Combo;
		Result.BestCombo = FMath::Max(Result.BestCombo, Result.Combo);
	}
	else
	{
		Result.Combo = 0;
		Judgement == EGuitarJudgement::Miss ? ++Result.Miss : ++Result.Wrong;
	}
}

double UGuitarPracticeSessionComponent::GetFeedbackAgeSeconds() const
{
	return FPlatformTime::Seconds() - FeedbackTimeSeconds;
}

int32 UGuitarPracticeSessionComponent::GetBestScore(FName SongId) const
{
	const FGuitarPracticeResult* Record = SavedRecords ? SavedRecords->Records.Find(SongId) : nullptr;
	return Record ? Record->Score : 0;
}

void UGuitarPracticeSessionComponent::SaveRecord()
{
	if (!SavedRecords) SavedRecords = NewObject<UGuitarPracticeSave>(this);
	FGuitarPracticeResult& Record = SavedRecords->Records.FindOrAdd(CurrentSong->GetFName());
	if (Result.Score >= Record.Score) Record = Result;
	bRecordSaved = UGameplayStatics::SaveGameToSlot(SavedRecords, SaveSlot, 0);
}
