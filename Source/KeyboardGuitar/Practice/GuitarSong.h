#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Instrument/GuitarTypes.h"
#include "GuitarSong.generated.h"

class UGuitarDefinition;

// 一首练习曲，两条时间线共用歌曲起点，时间单位为秒
UCLASS(BlueprintType)
class KEYBOARDGUITAR_API UGuitarSong : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song")
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song", meta = (ClampMin = "20", ClampMax = "240"))
	int32 BeatsPerMinute = 80;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Song", meta = (ClampMin = "1"))
	double DurationSeconds = 30.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart")
	TArray<FGuitarChordChangeEvent> ChordChanges;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chart")
	TArray<FGuitarPluckEvent> Plucks;

	// 查询谱面时刻的和弦；第一次切换前为空弦
	FName GetChordAt(double TimeSeconds) const;
	// 手写练习以第一次拨弦为第一小节的第一拍，保留前面的准备时间
	double GetBeatOriginSeconds() const { return Plucks.IsEmpty() ? 0.0 : Plucks[0].StartTimeSeconds; }
	// 在开始练习时验证外部谱面，不把非法音符带入运行流程
	bool ValidateChart(const UGuitarDefinition& Definition, FString& Error) const;
};
