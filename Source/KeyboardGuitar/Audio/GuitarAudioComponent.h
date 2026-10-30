#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarAudioComponent.generated.h"

class UAudioComponent;
class UGuitarDefinition;

// 每根弦独立管理一个声音实例；音高来自乐器组件，不读取输入或和弦状态
UCLASS(ClassGroup = (Guitar), meta = (BlueprintSpawnableComponent))
class KEYBOARDGUITAR_API UGuitarAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuitarAudioComponent();

	// 播放已解析的 MIDI 音高，同一根弦的新音符会替换旧音符
	UFUNCTION(BlueprintCallable, Category = "Guitar|Audio")
	void PlayNote(int32 StringIndex, int32 MidiNote);

	// 停止所有正在播放的音符
	UFUNCTION(BlueprintCallable, Category = "Guitar|Audio")
	void StopAll();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 吉他定义资产，包含和弦和音高映射
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guitar")
	TObjectPtr<UGuitarDefinition> GuitarDefinition;

private:
	// 每根弦的音频组件，用于播放对应的音符
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> StringVoices;
};
