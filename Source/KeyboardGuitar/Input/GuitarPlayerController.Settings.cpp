#include "GuitarPlayerController.h"
#include "Character/GuitarCharacter.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/GuitarScreenWidget.h"

void AGuitarPlayerController::HandleMenuKey()
{
	if (bGuitarMode)
	{
		PausePractice();
		return;
	}
	const AGuitarCharacter* ControlledCharacter = Cast<AGuitarCharacter>(GetPawn());
	if (!IsLocalController() || !GuitarScreenClass || !ControlledCharacter || ControlledCharacter->IsGuitarCameraTransitioning()) return;
	EnsureGuitarScreen();
	GetPawn()->ConsumeMovementInputVector();
	GetPawn()->GetMovementComponent()->StopMovementImmediately();
	RotationInput = FRotator::ZeroRotator;
	FlushPressedKeys();
	GuitarScreen->ShowSettings(true);
	GuitarScreen->AddToViewport();
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GuitarScreen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AGuitarPlayerController::CloseSettings()
{
	if (!bGuitarMode) HideGuitarScreen();
}

void AGuitarPlayerController::QuitToDesktop()
{
	// 独立运行退出游戏；PIE 中由引擎结束当前游玩，不关闭编辑器。
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
