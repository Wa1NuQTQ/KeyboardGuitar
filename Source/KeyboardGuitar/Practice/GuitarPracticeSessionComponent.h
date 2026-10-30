#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GuitarPracticeTypes.h"
#include "GuitarPracticeSessionComponent.generated.h"

class UGuitarSong;
class UGuitarDefinition;
struct FGuitarPlayedNote;

// 管理练习时间、暂停和判定；声音与动画仍由乐器组件负责
UCLASS(ClassGroup = (Guitar))
class KEYBOARDGUITAR_API UGuitarPracticeSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGuitarPracticeSessionComponent();

	// 流程入口；阶段变化统一通过 OnStageChanged 通知控制器和界面
	bool StartSong(UGuitarSong* Song, const UGuitarDefinition& Definition);
	void StartFreePlay();
	void SelectSongs();
	void Pause();
	void Resume();
	void JudgePluck(const FGuitarPlayedNote& Note);

	// 只读的时钟、判定和结算状态，UI 不自行累计时间或计算成绩
	double GetSongTimeSeconds() const;
	bool CanPlay() const { return Stage == EGuitarPracticeStage::Playing || Stage == EGuitarPracticeStage::FreePlay; }
	EGuitarPracticeStage GetStage() const { return Stage; }
	const UGuitarSong* GetSong() const { return CurrentSong; }
	const FGuitarPracticeResult& GetResult() const { return Result; }
	const TArray<EGuitarJudgement>& GetJudgements() const { return Judgements; }
	EGuitarJudgement GetLastJudgement() const { return LastJudgement; }
	double GetFeedbackAgeSeconds() const;
	int32 GetBestScore(FName SongId) const;
	const FString& GetError() const { return Error; }
	bool WasRecordSaved() const { return bRecordSaved; }

	FSimpleMulticastDelegate OnStageChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// 判定窗口按输入时刻计算，单位为秒
	UPROPERTY(EditDefaultsOnly, Category = "Practice", meta = (ClampMin = "0.01"))
	double PerfectWindowSeconds = 0.08;
	UPROPERTY(EditDefaultsOnly, Category = "Practice", meta = (ClampMin = "0.08"))
	double GoodWindowSeconds = 0.18;
	UPROPERTY(EditDefaultsOnly, Category = "Practice", meta = (ClampMin = "1"))
	double CountdownSeconds = 3.0;

private:
	void RefreshClock();
	void SetStage(EGuitarPracticeStage NewStage);
	void ApplyJudgement(int32 EventIndex, EGuitarJudgement Judgement);
	void ExpireNotes(double TimeSeconds);
	void SaveRecord();
	UPROPERTY(Transient)
	TObjectPtr<UGuitarSong> CurrentSong;
	UPROPERTY(Transient)
	TObjectPtr<UGuitarPracticeSave> SavedRecords;
	EGuitarPracticeStage Stage = EGuitarPracticeStage::Selecting;
	EGuitarPracticeStage StageBeforePause = EGuitarPracticeStage::Playing;
	FGuitarPracticeResult Result;
	TArray<EGuitarJudgement> Judgements;
	int32 NextExpiredEvent = 0;
	// 运行时钟使用单调时间，单位秒；暂停缓存谱面位置，恢复时重新定位起点
	double SongStartSeconds = 0.0;
	double PausedSongTimeSeconds = 0.0;
	double FeedbackTimeSeconds = -100.0;
	EGuitarJudgement LastJudgement = EGuitarJudgement::Pending;
	FString Error;
	bool bRecordSaved = true;
};
