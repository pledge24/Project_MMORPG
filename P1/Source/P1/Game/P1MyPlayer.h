// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/P1Player.h"
#include "InputActionValue.h"
#include "P1MyPlayer.generated.h"

/**
 *
 */
UCLASS()
class P1_API AP1MyPlayer : public AP1Player
{
	GENERATED_BODY()

public:
	AP1MyPlayer();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Tick(float DeltaTime) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

    virtual void Init(const Protocol::ObjectInfo& ObjectInfo_) override;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
    void NormalAttack(const FInputActionValue& Value);

    bool CanInputMovement() const;

protected:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    class UInputAction* NormalAttackAction;

    /** AttackSystemComponent*/
    UPROPERTY(EditAnywhere, Category = "Components")
    TSubclassOf<class UAttackSystemComponent> AttackSystemComponentClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UAttackSystemComponent* AttackSystemComponent;

private:
    /** MovePkt 전송 관련 */
    Protocol::C_MOVE MovePkt;

	const float MOVE_PACKET_SEND_DELAY = 0.2f;
    const float VELOCITY_TOLERANCE = 1.f;
    const float YAW_TOLERANCE = 30.f;
	float MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

	// Input Movement Cache.
	FVector2D DesiredInput;         // FInputActionValue
	FVector DesiredMoveDirection;   // 이동할 방향(Vector 타입)
	float DesiredYaw;               // 이동할 방향(Rotator 타입)

	// Dirty Flag
	FVector2D LastDesiredInput;
};
