#include "GuitarPlayerController.h"

#include "Character/GuitarCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "UI/GuitarScreenWidget.h"
#include "Practice/GuitarPracticeSessionComponent.h"
#include "Instrument/GuitarInstrumentComponent.h"
#include "InputCoreTypes.h"
#include "Misc/CoreDelegates.h"
#include "Audio/GuitarMetronomeComponent.h"
#include "Audio/GuitarBackgroundMusicComponent.h"

AGuitarPlayerController::AGuitarPlayerController()
{
	PracticeSession = CreateDefaultSubobject<UGuitarPracticeSessionComponent>(TEXT("PracticeSession"));
	Metronome = CreateDefaultSubobject<UGuitarMetronomeComponent>(TEXT("Metronome"));
	BackgroundMusic = CreateDefaultSubobject<UGuitarBackgroundMusicComponent>(TEXT("BackgroundMusic"));
	ChordRootActions.SetNum(7);
	ChordTypeActions.SetNum(8);
	PluckStringActions.SetNum(6);
}

void AGuitarPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BackgroundMusic->Synchronize(*PracticeSession);
	PracticeSession->OnStageChanged.AddUObject(this, &AGuitarPlayerController::HandlePracticeStageChanged);
	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &AGuitarPlayerController::PausePractice);

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
	bPlayerMappingContextAdded = true;
	SetInputMode(FInputModeGameOnly());
}

void AGuitarPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
	PracticeSession->OnStageChanged.RemoveAll(this);
	PracticeSession->SelectSongs();
	HideGuitarScreen();
	if (AGuitarCharacter* GuitarCharacter = Cast<AGuitarCharacter>(GetPawn()))
	{
		GuitarCharacter->OnGuitarCameraReady.RemoveAll(this);
	}
	GuitarScreen = nullptr;
	ClearHeldChords();

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	if (bGuitarMode && InputSubsystem)
	{
		InputSubsystem->RemoveMappingContext(GuitarMappingContext);
	}
	bGuitarMode = false;

	if (bPlayerMappingContextAdded && InputSubsystem)
	{
		InputSubsystem->RemoveMappingContext(PlayerMappingContext);
	}
	bPlayerMappingContextAdded = false;

	Super::EndPlay(EndPlayReason);
}

void AGuitarPlayerController::FlushPressedKeys()
{
	ClearHeldChords();
	Super::FlushPressedKeys();
}

void AGuitarPlayerController::OnUnPossess()
{
	SetGuitarMode(false);
	if (APawn* PreviousPawn = GetPawn())
		if (UGuitarInstrumentComponent* Instrument = PreviousPawn->FindComponentByClass<UGuitarInstrumentComponent>())
			Instrument->OnPluckResolved.RemoveAll(PracticeSession);
	HideGuitarScreen();
	if (AGuitarCharacter* GuitarCharacter = Cast<AGuitarCharacter>(GetPawn()))
	{
		GuitarCharacter->OnGuitarCameraReady.RemoveAll(this);
		GuitarCharacter->SetGuitarModePresentation(false);
	}
	// Super 会解除 Pawn，先清理旧人物上的和弦。
	ClearHeldChords();
	Super::OnUnPossess();
}

void AGuitarPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (UGuitarInstrumentComponent* Instrument = InPawn->FindComponentByClass<UGuitarInstrumentComponent>())
	{
		Instrument->OnPluckResolved.RemoveAll(PracticeSession);
		Instrument->OnPluckResolved.AddUObject(PracticeSession, &UGuitarPracticeSessionComponent::JudgePluck);
	}
	if (AGuitarCharacter* GuitarCharacter = Cast<AGuitarCharacter>(InPawn))
	{
		GuitarCharacter->OnGuitarCameraReady.AddUObject(this, &AGuitarPlayerController::HandleGuitarCameraReady);
		GuitarCharacter->SetGuitarModePresentation(bGuitarMode);
		if (bGuitarMode && GuitarCharacter->IsGuitarCameraReady())
		{
			ShowSongSelection();
		}
	}
}

void AGuitarPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	// 输入资产统一在 BP_GuitarPlayerController 中配置。
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGuitarPlayerController::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGuitarPlayerController::Look);
	EnhancedInputComponent->BindAction(ToggleGuitarModeAction, ETriggerEvent::Started, this, &AGuitarPlayerController::ToggleGuitarMode);

	BindGuitarActions(*EnhancedInputComponent);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AGuitarPlayerController::HandleMenuKey);
}

bool AGuitarPlayerController::CanControlCharacter() const
{
	const AGuitarCharacter* ControlledCharacter = Cast<AGuitarCharacter>(GetPawn());
	return !bGuitarMode && (!GuitarScreen || !GuitarScreen->IsInViewport())
		&& ControlledCharacter && !ControlledCharacter->IsGuitarCameraTransitioning();
}

void AGuitarPlayerController::Move(const FInputActionValue& Value)
{
	if (!CanControlCharacter())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();

	const FVector2D Input = Value.Get<FVector2D>();
	// 只用控制器的水平朝向，抬头和低头不改变行走平面。
	const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(RightDirection, Input.X);
	ControlledPawn->AddMovementInput(ForwardDirection, Input.Y);
}

void AGuitarPlayerController::Look(const FInputActionValue& Value)
{
	if (!CanControlCharacter())
	{
		return;
	}
	const FVector2D Input = Value.Get<FVector2D>();
	AddYawInput(Input.X);
	AddPitchInput(Input.Y);
}
