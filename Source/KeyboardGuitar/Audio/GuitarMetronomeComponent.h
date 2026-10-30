#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarMetronomeComponent.generated.h"

class UAudioComponent;
class USoundWave;
class UGuitarPracticeSessionComponent;

// 使用音频循环保持拍距，界面和游戏帧不逐拍触发声音
UCLASS(ClassGroup = (Guitar))
class KEYBOARDGUITAR_API UGuitarMetronomeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuitarMetronomeComponent();
	void Synchronize(const UGuitarPracticeSessionComponent& Session);
	void Toggle(const UGuitarPracticeSessionComponent& Session);
	bool IsEnabled() const { return bEnabled; }
	static double CalculateSourceOffset(double SongTimeSeconds, double BeatsPerMinute);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// 四秒、四拍的 60 BPM 循环；首拍重音，其余三拍轻音
	UPROPERTY(EditDefaultsOnly, Category = "Metronome")
	TObjectPtr<USoundWave> LoopSound;
	UPROPERTY(EditDefaultsOnly, Category = "Metronome", meta = (ClampMin = "0", ClampMax = "1"))
	float Volume = 0.45f;
	UPROPERTY(EditDefaultsOnly, Category = "Metronome", meta = (ClampMin = "20", ClampMax = "240"))
	int32 FreePlayBeatsPerMinute = 80;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Voice;
	bool bEnabled = true;
	bool bLoopRunning = false;
	int32 CurrentBeatsPerMinute = 0;
};
