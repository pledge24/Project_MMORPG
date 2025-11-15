// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "P1Player.h"
#include "InputActionValue.h"
#include "Logging/LogMacros.h"
#include "P1MyPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UInventoryComponent;
class UEquippedGearComponent;

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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Tick(float DeltaTime) override;

public:
    virtual void Initialize(const Protocol::ObjectInfo& InObjectInfo) override; // Server Only

    /** Getter함수 */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
    /** 상태 동기화용 함수 Delete */
    virtual void S_Move(float DeltaSeconds) override final {};
    virtual void S_NormalAttack(uint32 Combo) override final {};


protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
    void NormalAttack(const FInputActionValue& Value);
    void ToggleBattleMode(const FInputActionValue& Value);

    bool CanInputMovement() const;

public:
    /** 델리게이트 */
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExpChanged, TOptional<int32>, TOptional<int32>);
    FOnExpChanged OnExpChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, const int64);
    FOnGoldChanged OnGoldChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInvenSlotChanged, const Protocol::Slot&, bool);
    FOnInvenSlotChanged OnInvenSlotChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGearSlotChanged, const Protocol::Slot&);
    FOnGearSlotChanged OnGearSlotChanged;

    /** 패킷 수신 체크용 델리게이트 */
    DECLARE_MULTICAST_DELEGATE(FOnRecvBuyItemPkt);
    FOnRecvBuyItemPkt OnRecvBuyItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvSellItemPkt);
    FOnRecvSellItemPkt OnRecvSellItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUseItemPkt);
    FOnRecvUseItemPkt OnRecvUseItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvEquipGearPkt);
    FOnRecvEquipGearPkt OnRecvEquipGearPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUnequipGearPkt);
    FOnRecvUnequipGearPkt OnRecvUnequipGearPkt;

    /** ETC */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bBattleMode = false;

private:
    /**--------------------
     *       Camera
     *--------------------*/

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    /**--------------------
     *        Input
     *--------------------*/

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

    /** Attack Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* NormalAttackAction;

    /** Attack Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* ToggleBattleModeAction;

    /**--------------------
     *      Movement
     *--------------------*/

    /** MovePkt 전송 관련 */
    Protocol::C_MOVE MovePkt;

	const float MOVE_PACKET_SEND_DELAY = 0.2f;
    const float YAW_TOLERANCE = 60.f;
	float MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

	// Input Movement Cache.
	FVector2D DesiredInput;         // FInputActionValue
	FVector DesiredMoveDirection;   // 이동할 방향(Vector 타입)
	float DesiredYaw;               // 이동할 방향(Rotator 타입)

	// Dirty Flag
	FVector2D LastDesiredInput;

    // AvgSendSpeed DEBUGGING
    const bool Activate = false;
    int32 SendCounter = 1;
    float TotalSecond = 0.2f;
};
