#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GuitarPracticeTypes.generated.h"

UENUM()
enum class EGuitarPracticeStage : uint8
{
	Selecting,
	Countdown,
	Playing,
	FreePlay,
	Paused,
	Results
};

UENUM()
enum class EGuitarJudgement : uint8
{
	Pending,
	Perfect,
	Good,
	Wrong,
	Miss
};

// 一次练习的结果；错弦等额外输入会打断连击，但不会消耗其他音符
USTRUCT()
struct FGuitarPracticeResult
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Score = 0;
	UPROPERTY()
	int32 Combo = 0;
	UPROPERTY()
	int32 BestCombo = 0;
	UPROPERTY()
	int32 Perfect = 0;
	UPROPERTY()
	int32 Good = 0;
	UPROPERTY()
	int32 Wrong = 0;
	UPROPERTY()
	int32 Miss = 0;
};

// 仅保存每首曲目的最佳结果，不保存运行中的会话
UCLASS()
class UGuitarPracticeSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FName, FGuitarPracticeResult> Records;
};
