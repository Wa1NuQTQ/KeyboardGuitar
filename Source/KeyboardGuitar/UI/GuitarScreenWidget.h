#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Instrument/GuitarTypes.h"
#include "GuitarScreenWidget.generated.h"

class UButton;
class UWidgetSwitcher;
class UGuitarSong;
class UGuitarPracticeSessionComponent;
class USlider;
class UGuitarTabWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGuitarMusicVolumeChanged, float);

// 选歌、页面显示与按钮请求；模式和输入仍由 Controller 管理
UCLASS()
class KEYBOARDGUITAR_API UGuitarScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 控制器提供会话、曲目和演奏状态；界面负责显示，不反向决定玩法模式
	void Configure(UGuitarPracticeSessionComponent* InSession, const TArray<TObjectPtr<UGuitarSong>>& InSongs);
	void SetMetronomeEnabled(bool bEnabled);
	void ShowSongSelection();
	void ShowSessionPage();
	void ShowSettings(bool bFromMovement = false);
	void SetMusicVolume(float Volume);
	void SetChordType(EGuitarChordType Type);
	UGuitarSong* GetSelectedSong() const;

	// 操作请求由控制器订阅，界面不直接管理输入映射、相机或声音
	FSimpleMulticastDelegate OnExitRequested;
	FSimpleMulticastDelegate OnStartRequested;
	FSimpleMulticastDelegate OnFreePlayRequested;
	FSimpleMulticastDelegate OnPauseRequested;
	FSimpleMulticastDelegate OnResumeRequested;
	FSimpleMulticastDelegate OnRestartRequested;
	FSimpleMulticastDelegate OnSongSelectionRequested;
	FSimpleMulticastDelegate OnMetronomeToggleRequested;
	FSimpleMulticastDelegate OnSettingsClosed;
	FSimpleMulticastDelegate OnQuitRequested;
	FOnGuitarMusicVolumeChanged OnMusicVolumeChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	// 与 WBP_GuitarScreen 的 ScreenSwitcher 页序一致；None 表示从移动模式打开设置。
	enum class EPage
	{
		None = INDEX_NONE,
		SongSelect = 0,
		Countdown,
		Performance,
		Pause,
		Results,
		Settings
	};

	// 按钮与快捷键共用的操作入口
	UFUNCTION()
	void RequestExit();
	UFUNCTION()
	void Start();
	UFUNCTION()
	void FreePlay();
	UFUNCTION()
	void PreviousSong();
	UFUNCTION()
	void NextSong();
	UFUNCTION()
	void Pause();
	UFUNCTION()
	void Resume();
	UFUNCTION()
	void Restart();
	UFUNCTION()
	void SelectSongs();
	UFUNCTION()
	void ToggleMetronome();
	UFUNCTION()
	void OpenSettings();
	UFUNCTION()
	void CloseSettings();
	UFUNCTION()
	void Quit();
	UFUNCTION()
	void ChangeMusicVolume(float Volume);
	void BindSettingsControls();

	// 选歌卡片的横移和淡入淡出；快速连续切换按顺序处理
	void QueueSongChange(int32 Direction);
	void UpdateSongTransition();
	void FinishSongTransition();
	void RestoreSongCards();
	void CacheSongCards();

	// 只更新显示；数据读取集中在 Display.cpp
	void RefreshSongCards();
	void RefreshPerformance(UUserWidget* Page);
	void RefreshResults();
	void SetText(UUserWidget* Page, FName Name, const FString& Value);
	UUserWidget* GetPage(EPage Page) const;

	// 蓝图布局入口；实例引用只在运行中持有，不写回资产
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;
	UPROPERTY(Transient)
	TObjectPtr<UGuitarPracticeSessionComponent> Session;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGuitarSong>> Songs;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> BoundButtons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SongCards;
	UPROPERTY(Transient)
	TObjectPtr<USlider> MusicVolumeSlider;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGuitarTabWidget>> Tabs;

	// 设置与选歌的显示状态，与练习阶段分开保存
	EGuitarChordType SelectedChordType = EGuitarChordType::Major;
	EPage SettingsReturnPage = EPage::SongSelect;
	float MusicVolume = 0.18f;
	TArray<FVector2D> SongCardTranslations;
	TArray<float> SongCardOpacities;
	int32 SelectedSongIndex = 0;
	int32 SongTransitionDirection = 0;
	int32 PendingSongSteps = 0;
	double SongTransitionStartSeconds = 0.0;
	bool bSongTransitionActive = false;
	bool bSongTransitionCommitted = false;
	bool bMetronomeEnabled = true;
	bool bExitOnTabRelease = false;
};
