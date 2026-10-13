#include "GuitarCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AGuitarCharacter::AGuitarCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 弹簧臂设置
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 200.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 35.0f, 60.0f);
	CameraBoom->bUsePawnControlRotation = true;

	// 相机设置
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 让相机不跟随角色旋转

	// 禁用角色旋转
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 角色移动设置
	GetCharacterMovement()->bOrientRotationToMovement = true; // 角色移动时面向移动方向
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 角色旋转速度
	GetCharacterMovement()->MaxWalkSpeed = 200.0f; // 角色移动速度

}

void AGuitarCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGuitarCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGuitarCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

