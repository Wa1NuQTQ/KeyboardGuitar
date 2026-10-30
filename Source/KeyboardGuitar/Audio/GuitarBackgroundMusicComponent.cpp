#include "GuitarBackgroundMusicComponent.h"
#include "Components/AudioComponent.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Sound/SoundBase.h"

UGuitarBackgroundMusicComponent::UGuitarBackgroundMusicComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGuitarBackgroundMusicComponent::BeginPlay()
{
	Super::BeginPlay();
	Voice = NewObject<UAudioComponent>(GetOwner(), TEXT("BackgroundMusicVoice"));
	Voice->bAutoActivate = false;
	Voice->bAutoDestroy = false;
	Voice->bIsUISound = true;
	Voice->RegisterComponent();
	Voice->SetSound(Track);
	Voice->SetVolumeMultiplier(Volume);
}

void UGuitarBackgroundMusicComponent::Synchronize(const UGuitarPracticeSessionComponent& Session)
{
	if (!Voice) return;
	const bool bShouldPlay = Session.GetStage() == EGuitarPracticeStage::Selecting
		|| Session.GetStage() == EGuitarPracticeStage::Results;
	if (bPlaying == bShouldPlay) return;
	bPlaying = bShouldPlay;
	if (bPlaying) Voice->FadeIn(0.8f, 1.0f);
	else Voice->FadeOut(0.4f, 0.0f);
}

void UGuitarBackgroundMusicComponent::SetVolume(float InVolume)
{
	Volume = FMath::Clamp(InVolume, 0.0f, 1.0f);
	if (Voice) Voice->SetVolumeMultiplier(Volume);
}

void UGuitarBackgroundMusicComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Voice)
	{
		Voice->Stop();
		Voice->DestroyComponent();
		Voice = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
