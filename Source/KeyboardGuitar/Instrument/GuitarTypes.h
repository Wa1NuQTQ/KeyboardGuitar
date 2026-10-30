#pragma once

#include "CoreMinimal.h"
#include "GuitarTypes.generated.h"

// 和弦类型；顺序与输入动作数组和类型配色表一致
UENUM(BlueprintType)
enum class EGuitarChordType : uint8
{
	Major = 0 UMETA(DisplayName = "大三和弦"),
	Minor = 1 UMETA(DisplayName = "小三和弦"),
	DominantSeventh = 2 UMETA(DisplayName = "属七和弦"),
	MajorSeventh = 3 UMETA(DisplayName = "大七和弦"),
	MinorSeventh = 4 UMETA(DisplayName = "小七和弦"),
	SuspendedSecond = 5 UMETA(DisplayName = "挂二和弦"),
	SuspendedFourth = 6 UMETA(DisplayName = "挂四和弦"),
	AddedNinth = 7 UMETA(DisplayName = "加九和弦")
};

// 全项目统一的弦数与和弦标识
namespace Guitar
{
	inline constexpr int32 StringCount = 6;

	// 根音以 C=0、B=11 表示；统一生成和弦表中的标识
	KEYBOARDGUITAR_API FName MakeChordId(int32 RootPitchClass, EGuitarChordType Type);
}

// 左手按弦手指
UENUM(BlueprintType)
enum class EGuitarFinger : uint8
{
	None = 0 UMETA(DisplayName = "未指定"),
	Index = 1 UMETA(DisplayName = "食指"),
	Middle = 2 UMETA(DisplayName = "中指"),
	Ring = 3 UMETA(DisplayName = "无名指"),
	Little = 4 UMETA(DisplayName = "小指")
};

// 一根弦的左手指法
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarStringFingering
{
	GENERATED_BODY()

	// 品位；0 表示空弦，制音由 bMuted 单独表示
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fingering", meta = (ClampMin = "0", EditCondition = "!bMuted"))
	int32 Fret = 0;
	// 制音弦仍有拨弦动作，但不发出音符
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fingering")
	bool bMuted = false;
	// 负责按弦的手指；空弦与制音弦不需要手指
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fingering", meta = (EditCondition = "!bMuted && Fret > 0"))
	EGuitarFinger Finger = EGuitarFinger::None;
};

// 预设和弦
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarChord
{
	GENERATED_BODY()

	// 默认创建六根弦的指法
	FGuitarChord()
	{
		StringFingerings.SetNum(Guitar::StringCount);
	}
	// 和弦的唯一标识，同时用于查询和显示
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chord")
	FName ChordId = NAME_None;
	// 固定六项；索引 0 为最细的第一弦，5 为最粗的第六弦
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, Category = "Chord")
	TArray<FGuitarStringFingering> StringFingerings;
};

// 曲谱左手时间线
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarChordChangeEvent
{
	GENERATED_BODY()

	// 从曲目起点计算的切换时间，单位秒；保持到下一次切换
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	double StartTimeSeconds = 0.0;
	// 目标和弦；NAME_None 表示空弦
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chord")
	FName ChordId = NAME_None;
};

// 曲谱右手时间线
USTRUCT(BlueprintType)
struct KEYBOARDGUITAR_API FGuitarPluckEvent
{
	GENERATED_BODY()

	// 从曲目起点计算的拨弦时间，单位秒；判定窗口由练习会话管理
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = "0.0"))
	double StartTimeSeconds = 0.0;
	// 弦索引 0～5 对应第一弦到第六弦，不保存重复的和弦或品位
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pluck", meta = (ClampMin = "0", ClampMax = "5"))
	int32 StringIndex = 0;
};
