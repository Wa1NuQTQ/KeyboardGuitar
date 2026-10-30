#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Instrument/GuitarTypes.h"
#include "GuitarPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class UGuitarScreenWidget;
class UGuitarPracticeSessionComponent;
class UGuitarSong;
class UGuitarMetronomeComponent;
class UGuitarBackgroundMusicComponent;
struct FInputActionValue;

// 协调输入、模式和练习流程；音高交给乐器，姿势和镜头交给人物
UCLASS()
class KEYBOARDGUITAR_API AGuitarPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGuitarPlayerController();
	UGuitarPracticeSessionComponent* GetPracticeSession() const { return PracticeSession; }
	UGuitarMetronomeComponent* GetMetronome() const { return Metronome; }
	// 失焦或切换输入模式时清理按住的和弦
	virtual void FlushPressedKeys() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void OnUnPossess() override;
	virtual void OnPossess(APawn* InPawn) override;

	// 移动、视角和模式切换的输入映射
	UPROPERTY(EditDefaultsOnly, Category = "Input|Player")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	// 处理玩家移动输入
	UPROPERTY(EditDefaultsOnly, Category = "Input|Player")
	TObjectPtr<UInputAction> MoveAction;

	// 处理玩家视角输入
	UPROPERTY(EditDefaultsOnly, Category = "Input|Player")
	TObjectPtr<UInputAction> LookAction;

	// 根音、和弦类型和拨弦的输入映射
	UPROPERTY(EditDefaultsOnly, Category = "Input|Guitar|Mode")
	TObjectPtr<UInputMappingContext> GuitarMappingContext;

	// 处理吉他模式切换输入
	UPROPERTY(EditDefaultsOnly, Category = "Input|Guitar|Mode")
	TObjectPtr<UInputAction> ToggleGuitarModeAction;

	// 根音动作依次对应 C、D、E、F、G、A、B
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "Input|Guitar|Chords")
	TArray<TObjectPtr<UInputAction>> ChordRootActions;

	// 和弦类型动作顺序与 EGuitarChordType 一致
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "Input|Guitar|Chords")
	TArray<TObjectPtr<UInputAction>> ChordTypeActions;

	// 拨弦动作依次对应第一弦到第六弦
	UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "Input|Guitar|Plucking")
	TArray<TObjectPtr<UInputAction>> PluckStringActions;

	// 和弦升半音输入
	UPROPERTY(EditDefaultsOnly, Category = "Input|Guitar|Chords")
	TObjectPtr<UInputAction> ChordSharpAction;

	// 处理吉他和弦降半音输入
	UPROPERTY(EditDefaultsOnly, Category = "Input|Guitar|Chords")
	TObjectPtr<UInputAction> ChordFlatAction;

	// 选歌和演奏页面的 UI 类
	UPROPERTY(EditDefaultsOnly, Category = "UI|Guitar")
	TSubclassOf<UGuitarScreenWidget> GuitarScreenClass;

	// 曲目通过蓝图配置，随 Controller 一起打包
	UPROPERTY(EditDefaultsOnly, Category = "Practice")
	TArray<TObjectPtr<UGuitarSong>> Songs;

private:
	// 练习流程：GuitarPlayerController.Practice.cpp
	void StartSelectedSong();
	void StartFreePlay();
	void PausePractice();
	void ResumePractice();
	void RestartPractice();
	void ReturnToSongSelection();
	void HandlePracticeStageChanged();
	bool CanPlayGuitar() const;
	void StopGuitarSounds();
	void ToggleMetronome();
	UPROPERTY(VisibleAnywhere, Category = "Audio")
	TObjectPtr<UGuitarBackgroundMusicComponent> BackgroundMusic;
	UPROPERTY(VisibleAnywhere, Category = "Practice")
	TObjectPtr<UGuitarPracticeSessionComponent> PracticeSession;
	UPROPERTY(VisibleAnywhere, Category = "Practice")
	TObjectPtr<UGuitarMetronomeComponent> Metronome;

	// 输入绑定与移动：GuitarPlayerController.cpp
	bool CanControlCharacter() const;

	// 处理玩家移动输入
	void Move(const FInputActionValue& Value);

	// 处理玩家视角输入
	void Look(const FInputActionValue& Value);

	// 模式和选歌界面：GuitarPlayerController.Mode.cpp
	void ToggleGuitarMode();

	// 更新模式，同步人物姿势和镜头
	void SetGuitarMode(bool bEnabled);

	// 镜头到位后打开选歌页
	void HandleGuitarCameraReady();
	// 显示选歌界面并接管鼠标和键盘
	void ShowSongSelection();
	void EnsureGuitarScreen();
	// 隐藏界面并归还游戏输入
	void HideGuitarScreen();
	// 处理界面发出的返回请求
	void ReturnToMovement();

	// 共用设置与退出：GuitarPlayerController.Settings.cpp
	void HandleMenuKey();
	void CloseSettings();
	void QuitToDesktop();

	// 和弦与拨弦：GuitarPlayerController.Guitar.cpp
	void BindGuitarActions(UEnhancedInputComponent& EnhancedInputComponent);
	// 根音按下时保存当前类型和升降号
	void ChooseChordRoot(int32 RootPitchClass);
	// 松开根音后恢复仍按住的上一个和弦
	void ReleaseChordRoot(int32 RootPitchClass);
	// 清理按住状态并恢复空弦
	void ClearHeldChords();
	// 选择下一次按下根音时使用的和弦类型
	void ChooseChordType(EGuitarChordType Type);
	// 同步本帧根音状态后拨响指定琴弦
	void PluckString(int32 StringIndex);

	struct FHeldChord
	{
		int32 RootPitchClass;
		FName ChordId;
	};

	// 按下时保存和弦快照，最后一项优先；松开后可恢复仍按住的上一项。
	TArray<FHeldChord> HeldChords;

	// 运行时创建并重复使用的界面实例
	UPROPERTY(Transient)
	TObjectPtr<UGuitarScreenWidget> GuitarScreen;

	// 只在结束时移除本控制器注册的输入映射
	bool bPlayerMappingContextAdded = false;
	// 当前模式和待选和弦类型
	bool bGuitarMode = false;
	EGuitarChordType PendingChordType = EGuitarChordType::Major;
};
