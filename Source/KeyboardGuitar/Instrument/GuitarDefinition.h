#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GuitarTypes.h"
#include "GuitarDefinition.generated.h"

class USoundWave;

// 吉他配置资产，保存调弦、和弦和音符采样
UCLASS(BlueprintType)
class KEYBOARDGUITAR_API UGuitarDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UGuitarDefinition();

	// 找到指定和弦标识的预设和弦
	const FGuitarChord* FindChordById(FName ChordId) const;

	// 吉他每根空弦的 MIDI 音符，0 为第一弦高音 E，5 为第六弦低音 E
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Guitar")
	TArray<int32> OpenStringMidiNotes;

	// 可选择的预设和弦
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guitar")
	TArray<FGuitarChord> Chords;

	// MIDI 音高到吉他单音采样的映射。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TMap<int32, TObjectPtr<USoundWave>> NoteSamples;
};
