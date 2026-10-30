#include "GuitarAudioComponent.h"

#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "Instrument/GuitarDefinition.h"
#include "Sound/SoundWave.h"

UGuitarAudioComponent::UGuitarAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGuitarAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	StringVoices.Reserve(Guitar::StringCount);

	for (int32 StringIndex = 0; StringIndex < Guitar::StringCount; ++StringIndex)
	{
		UAudioComponent* Voice = NewObject<UAudioComponent>(GetOwner());

		Voice->bAutoActivate = false;
		Voice->bAutoDestroy = false;

		Voice->RegisterComponent();
		StringVoices.Add(Voice);
	}
}

void UGuitarAudioComponent::PlayNote(int32 StringIndex, int32 MidiNote)
{
	USoundWave* Sample = GuitarDefinition->NoteSamples.FindRef(MidiNote).Get();

	UAudioComponent* Voice = StringVoices[StringIndex];
	Voice->Stop();
	Voice->SetSound(Sample);
	Voice->Play();
}

void UGuitarAudioComponent::StopAll()
{
	for (UAudioComponent* Voice : StringVoices)
	{
		Voice->Stop();
	}
}

void UGuitarAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAll();

	for (UAudioComponent* Voice : StringVoices)
	{
		Voice->DestroyComponent();
	}

	StringVoices.Reset();

	Super::EndPlay(EndPlayReason);
}
