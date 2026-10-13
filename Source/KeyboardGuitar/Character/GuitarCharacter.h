#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GuitarCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class KEYBOARDGUITAR_API AGuitarCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGuitarCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))	
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
};
