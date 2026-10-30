#include "GuitarPlayerController.h"

#include "Character/GuitarCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UI/GuitarScreenWidget.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Audio/GuitarMetronomeComponent.h"
#include "Audio/GuitarBackgroundMusicComponent.h"

void AGuitarPlayerController::ToggleGuitarMode()
{
	if (bGuitarMode && (PracticeSession->CanPlay() || PracticeSession->GetStage() == EGuitarPracticeStage::Countdown))
		PausePractice();
	else SetGuitarMode(!bGuitarMode);
}

void AGuitarPlayerController::SetGuitarMode(bool bEnabled)
{
	if (bGuitarMode == bEnabled)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	bGuitarMode = bEnabled;
	PracticeSession->SelectSongs();
	StopGuitarSounds();
	ClearHeldChords();
	if (bGuitarMode)
	{
		// 清掉本帧已排队的鼠标输入，返回时保持进入前的朝向。
		RotationInput = FRotator::ZeroRotator;
	}
	// 映射由练习阶段接管，选歌和倒数期间不接受演奏输入。
	InputSubsystem->RemoveMappingContext(GuitarMappingContext);
	if (AGuitarCharacter* GuitarCharacter = Cast<AGuitarCharacter>(GetPawn()))
	{
		GuitarCharacter->SetGuitarModePresentation(bGuitarMode);
	}
	if (bGuitarMode)
	{
		if (APawn* ControlledPawn = GetPawn())
		{
			ControlledPawn->ConsumeMovementInputVector();
			if (UPawnMovementComponent* Movement = ControlledPawn->GetMovementComponent())
			{
				Movement->StopMovementImmediately();
			}
			if (const AGuitarCharacter* GuitarCharacter = Cast<AGuitarCharacter>(ControlledPawn);
				GuitarCharacter && GuitarCharacter->IsGuitarCameraReady())
			{
				ShowSongSelection();
			}
		}
	}
	else
	{
		HideGuitarScreen();
	}
}

void AGuitarPlayerController::HandleGuitarCameraReady()
{
	if (bGuitarMode)
	{
		ShowSongSelection();
	}
}

void AGuitarPlayerController::ShowSongSelection()
{
	if (!IsLocalController() || !GuitarScreenClass || !GetPawn())
	{
		return;
	}
	EnsureGuitarScreen();
	if (!GuitarScreen || GuitarScreen->IsInViewport()) return;
	GuitarScreen->ShowSongSelection();
	GuitarScreen->AddToViewport();
	// 进入时的 Tab 释放可能被菜单接管，清理跨页面持键。
	FlushPressedKeys();
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GuitarScreen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AGuitarPlayerController::EnsureGuitarScreen()
{
	if (!GuitarScreen)
	{
		GuitarScreen = CreateWidget<UGuitarScreenWidget>(this, GuitarScreenClass);
		GuitarScreen->Configure(PracticeSession, Songs);
		GuitarScreen->SetMetronomeEnabled(Metronome->IsEnabled());
		GuitarScreen->SetMusicVolume(BackgroundMusic->GetVolume());
		GuitarScreen->SetChordType(PendingChordType);
		GuitarScreen->OnExitRequested.AddUObject(this, &AGuitarPlayerController::ReturnToMovement);
		GuitarScreen->OnStartRequested.AddUObject(this, &AGuitarPlayerController::StartSelectedSong);
		GuitarScreen->OnFreePlayRequested.AddUObject(this, &AGuitarPlayerController::StartFreePlay);
		GuitarScreen->OnPauseRequested.AddUObject(this, &AGuitarPlayerController::PausePractice);
		GuitarScreen->OnResumeRequested.AddUObject(this, &AGuitarPlayerController::ResumePractice);
		GuitarScreen->OnRestartRequested.AddUObject(this, &AGuitarPlayerController::RestartPractice);
		GuitarScreen->OnSongSelectionRequested.AddUObject(this, &AGuitarPlayerController::ReturnToSongSelection);
		GuitarScreen->OnMetronomeToggleRequested.AddUObject(this, &AGuitarPlayerController::ToggleMetronome);
		GuitarScreen->OnSettingsClosed.AddUObject(this, &AGuitarPlayerController::CloseSettings);
		GuitarScreen->OnQuitRequested.AddUObject(this, &AGuitarPlayerController::QuitToDesktop);
		GuitarScreen->OnMusicVolumeChanged.AddUObject(BackgroundMusic, &UGuitarBackgroundMusicComponent::SetVolume);
	}
}

void AGuitarPlayerController::HideGuitarScreen()
{
	if (GuitarScreen)
	{
		GuitarScreen->RemoveFromParent();
	}
	if (IsLocalController())
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AGuitarPlayerController::ReturnToMovement()
{
	SetGuitarMode(false);
}
