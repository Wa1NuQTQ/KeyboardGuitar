#include "GuitarMetronomeComponent.h"
#include "Components/AudioComponent.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Practice/GuitarSong.h"
#include "Sound/SoundWave.h"

namespace
{
	constexpr double SourceBeatsPerMinute = 60.0;
	constexpr double SourceLoopSeconds = 4.0;
}

UGuitarMetronomeComponent::UGuitarMetronomeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGuitarMetronomeComponent::BeginPlay()
{
	Super::BeginPlay();
	Voice = NewObject<UAudioComponent>(GetOwner());
	Voice->bAutoActivate = false;
	Voice->bAutoDestroy = false;
	Voice->bIsUISound = true;
	Voice->RegisterComponent();
	Voice->SetSound(LoopSound);
	Voice->SetVolumeMultiplier(Volume);
}

double UGuitarMetronomeComponent::CalculateSourceOffset(double SongTimeSeconds, double BeatsPerMinute)
{
	const double Offset = FMath::Fmod(SongTimeSeconds * BeatsPerMinute / SourceBeatsPerMinute, SourceLoopSeconds);
	return Offset < 0.0 ? Offset + SourceLoopSeconds : Offset;
}

void UGuitarMetronomeComponent::Synchronize(const UGuitarPracticeSessionComponent& Session)
{
	if (!Voice) return;
	const bool bShouldPlay = bEnabled && (Session.GetStage() == EGuitarPracticeStage::Countdown || Session.CanPlay());
	if (!bShouldPlay)
	{
		Voice->Stop();
		bLoopRunning = false;
		return;
	}
	const int32 Tempo = Session.GetSong() ? Session.GetSong()->BeatsPerMinute : FreePlayBeatsPerMinute;
	// 倒数进入演奏时延续同一循环；暂停恢复和重新开始时按当前谱面位置寻址
	if (!bLoopRunning || CurrentBeatsPerMinute != Tempo || Session.GetStage() == EGuitarPracticeStage::Countdown
		|| Session.GetStage() == EGuitarPracticeStage::FreePlay)
	{
		Voice->Stop();
		Voice->SetPitchMultiplier(Tempo / SourceBeatsPerMinute);
		const double BeatOrigin = Session.GetSong() ? Session.GetSong()->GetBeatOriginSeconds() : 0.0;
		Voice->Play(CalculateSourceOffset(Session.GetSongTimeSeconds() - BeatOrigin, Tempo));
		CurrentBeatsPerMinute = Tempo;
		bLoopRunning = true;
	}
}

void UGuitarMetronomeComponent::Toggle(const UGuitarPracticeSessionComponent& Session)
{
	bEnabled = !bEnabled;
	Synchronize(Session);
}

void UGuitarMetronomeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Voice)
	{
		Voice->Stop();
		Voice->DestroyComponent();
		Voice = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
