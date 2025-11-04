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
    virtual void Initialize(const Protocol::ObjectInfo& InObjectInfo) override;

    /** Getter함수 */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    UInventoryComponent* GetInventory() const { return InventoryComponent; }
    UEquippedGearComponent* GetEquippedGear() const { return EquippedGearComponent; }

    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_PlayerInfo; }
    int32 GetGold() const { return _PlayerInfo->gold(); };
    int32 GetPlayerLevel() const { return _PlayerInfo->level(); };
    uint64 GetPlayerId() const { return _PlayerId; }

    /** 패킷 핸들 함수 */
    void HandleGoldChanged(int64 Gold);
    void HandleLevelChanged(int32 Level);
    void HandleExpChanged(int32 CurExp, int32 MaxExp);
    void HandleStatChanged(const Protocol::StatInfo& InStatInfo);

protected:
    /** 상태 동기화용 함수 Delete */
    virtual void S_Move(float DeltaSeconds) override final {};
    virtual void S_NormalAttack(uint32 Combo) override final {};


protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
    void NormalAttack(const FInputActionValue& Value);

    bool CanInputMovement() const;

public:
    /** 델리게이트 */
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExpChanged, TOptional<int32>, TOptional<int32>);
    FOnExpChanged OnExpChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, const int32);
    FOnGoldChanged OnGoldChanged;

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

protected:
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

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    UInputAction* NormalAttackAction;

    /**--------------------
     *      Components
     *--------------------*/

    /** Inventory Component*/
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInventoryComponent* InventoryComponent;

    /** EquippedGear Component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UEquippedGearComponent* EquippedGearComponent;

private:
    uint64 _PlayerId = 0;
    Protocol::PlayerInfo* _PlayerInfo;
    Protocol::StatInfo* _StatInfo;

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
