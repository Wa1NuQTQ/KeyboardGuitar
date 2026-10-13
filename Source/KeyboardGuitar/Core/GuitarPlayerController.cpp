// Fill out your copyright notice in the Description page of Project Settings.


#include "GuitarPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"

void AGuitarPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 获取本地玩家
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	// 添加输入映射上下文到本地玩家的输入子系统
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubsystem) return;

	// 将玩家映射上下文添加到输入子系统中，优先级为0
	InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
	SetInputMode(FInputModeGameOnly());
}

void AGuitarPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent) return;
	
	// 绑定移动和视角操作到对应的处理函数
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGuitarPlayerController::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGuitarPlayerController::Look);
}

void AGuitarPlayerController::Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	const FVector2D Input = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDirection, Input.Y);
	ControlledPawn->AddMovementInput(RightDirection, Input.X);
}

void AGuitarPlayerController::Look(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	const FVector2D Input = Value.Get<FVector2D>();
	ControlledPawn->AddControllerYawInput(Input.X);
	ControlledPawn->AddControllerPitchInput(Input.Y);
}
