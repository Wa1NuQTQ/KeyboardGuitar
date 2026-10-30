#include "GuitarPlayerController.h"
#include "Audio/GuitarAudioComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "UI/GuitarScreenWidget.h"
#include "Audio/GuitarMetronomeComponent.h"
#include "Audio/GuitarBackgroundMusicComponent.h"

bool AGuitarPlayerController::CanPlayGuitar() const
{
	return bGuitarMode && PracticeSession->CanPlay();
}

void AGuitarPlayerController::StopGuitarSounds()
{
	if (APawn* ControlledPawn = GetPawn())
		if (UGuitarAudioComponent* Audio = ControlledPawn->FindComponentByClass<UGuitarAudioComponent>())
			Audio->StopAll();
}

void AGuitarPlayerController::StartSelectedSong()
{
	if (!GuitarScreen || !GetPawn()) return;
	const UGuitarInstrumentComponent* Instrument = GetPawn()->FindComponentByClass<UGuitarInstrumentComponent>();
	if (!PracticeSession->StartSong(GuitarScreen->GetSelectedSong(), *Instrument->GetDefinition()))
		ClientMessage(PracticeSession->GetError());
}

void AGuitarPlayerController::StartFreePlay()
{
	PracticeSession->StartFreePlay();
}

void AGuitarPlayerController::PausePractice()
{
	if (bGuitarMode) PracticeSession->Pause();
}

void AGuitarPlayerController::ResumePractice()
{
	PracticeSession->Resume();
}

void AGuitarPlayerController::RestartPractice()
{
	if (PracticeSession->GetSong()) StartSelectedSong();
	else StartFreePlay();
}

void AGuitarPlayerController::ReturnToSongSelection()
{
	PracticeSession->SelectSongs();
}

void AGuitarPlayerController::HandlePracticeStageChanged()
{
	Metronome->Synchronize(*PracticeSession);
	BackgroundMusic->Synchronize(*PracticeSession);
	if (!bGuitarMode || !GuitarScreen || !GuitarScreen->IsInViewport()) return;
	GuitarScreen->ShowSessionPage();
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	InputSubsystem->RemoveMappingContext(GuitarMappingContext);
	// 清空跨页面按住的输入，恢复时需要重新按下根音键
	FlushPressedKeys();
	if (PracticeSession->CanPlay()) InputSubsystem->AddMappingContext(GuitarMappingContext, 1);
	else StopGuitarSounds();

	const bool bMenu = PracticeSession->GetStage() == EGuitarPracticeStage::Selecting
		|| PracticeSession->GetStage() == EGuitarPracticeStage::Paused || PracticeSession->GetStage() == EGuitarPracticeStage::Results;
	bShowMouseCursor = true;
	if (bMenu)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GuitarScreen->TakeWidget());
		SetInputMode(InputMode);
	}
	else
	{
		// UI 未处理的字母和数字交给 Enhanced Input；节拍器按钮仍可点击
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GuitarScreen->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

void AGuitarPlayerController::ToggleMetronome()
{
	Metronome->Toggle(*PracticeSession);
	if (GuitarScreen) GuitarScreen->SetMetronomeEnabled(Metronome->IsEnabled());
}
