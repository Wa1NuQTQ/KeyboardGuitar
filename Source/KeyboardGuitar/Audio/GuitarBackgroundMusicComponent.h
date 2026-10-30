#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarBackgroundMusicComponent.generated.h"

class UAudioComponent;
class USoundBase;
class UGuitarPracticeSessionComponent;

// 环境音乐只陪伴移动、选歌和结算，演奏时淡出以免干扰听音。
UCLASS(ClassGroup = (Guitar))
class KEYBOARDGUITAR_API UGuitarBackgroundMusicComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuitarBackgroundMusicComponent();
	void Synchronize(const UGuitarPracticeSessionComponent& Session);
	void SetVolume(float InVolume);
	float GetVolume() const { return Volume; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(EditDefaultsOnly, Category = "Music")
	TObjectPtr<USoundBase> Track;
	UPROPERTY(EditDefaultsOnly, Category = "Music", meta = (ClampMin = "0", ClampMax = "1"))
	float Volume = 0.18f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Voice;
	bool bPlaying = false;
};
