#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarInstrumentComponent.generated.h"

class UGuitarDefinition;
struct FGuitarChord;

// 输入瞬间的演奏快照，后续换和弦不会改变已经发出的音符
struct KEYBOARDGUITAR_API FGuitarPlayedNote
{
	FName ChordId = NAME_None;
	int32 StringIndex = 0;
	int32 Fret = 0;
	int32 MidiNote = 0;
	bool bMuted = false;
	double InputTimeSeconds = 0.0;
};

// 动作、声音和判定共享同一次输入解析，订阅者不重复计算音高
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGuitarNotePlayed, int32 /* StringIndex */, int32 /* MidiNote */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGuitarStringPlucked, int32 /* StringIndex */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGuitarPluckResolved, const FGuitarPlayedNote&);

// 当前和弦的唯一写入者，负责指法与音高；不操作骨骼或音频实例
UCLASS(ClassGroup = (Guitar), meta = (BlueprintSpawnableComponent))
class KEYBOARDGUITAR_API UGuitarInstrumentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuitarInstrumentComponent();

	// 选择和弦只改变指法，不触发声音
	void SelectChord(FName ChordId);

	// 恢复空弦，不截断已经响起的余音
	void ClearChord();

	// 获取当前选择的和弦
	const FGuitarChord* GetCurrentChord() const;
	FName GetCurrentChordId() const { return CurrentChordId; }
	const UGuitarDefinition* GetDefinition() const { return GuitarDefinition; }

	// 解析弦索引 0～5，先保存输入快照，再广播动作、声音和判定事件
	void PluckString(int32 StringIndex);

	// 吉他演奏事件的委托
	FOnGuitarNotePlayed OnNotePlayed;
	FOnGuitarStringPlucked OnStringPlucked;
	FOnGuitarPluckResolved OnPluckResolved;

protected:
	// 吉他定义资产，包含和弦和音高映射
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guitar")
	TObjectPtr<UGuitarDefinition> GuitarDefinition;

	// 当前选择的和弦标识
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guitar")
	FName CurrentChordId = NAME_None;
};
