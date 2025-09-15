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
    ~AP1MyPlayer() override;

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

    const Protocol::PlayerInfo& GetPlayerInfo() { return *_PlayerInfo; }

    /** 레벨 관련 */
    void SetLevel(int32 Level_);
    void SetExp(int32 CurExp, int32 MaxExp=-1);

    /** 스텟 관련 */
    void SetStatInfo(const Protocol::StatInfo& StatInfo_);

    /** 소유 관련 */
    void SetGold(int64 Gold);
    int32 GetGold() { return _PlayerInfo->gold(); };
    int32 GetLevel() { return _PlayerInfo->level(); };

    void SetInventorySlot(const Protocol::Slot& Slot_, bool OnUse = false);
    void SetEquippedGearSlot(const Protocol::Slot& Slot_);

public:
    /** 델리게이트 모음(위젯 상태 갱신용) */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32);
    FOnLevelChanged OnLevelChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExpChanged, int32, int32);
    FOnExpChanged OnExpChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatInfoChanged, const Protocol::StatInfo&);
    FOnStatInfoChanged OnStatInfoChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, const int32);
    FOnGoldChanged OnGoldChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotChanged, const Protocol::Slot&, bool);
    FOnInventorySlotChanged OnInventorySlotChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedGearSlotChanged, const Protocol::Slot&);
    FOnEquippedGearSlotChanged OnEquippedGearSlotChanged;

    /** 델리게이트 모음(위젯 액션 알림용) */
    DECLARE_MULTICAST_DELEGATE(FOnRep_BuyItem);
    FOnRep_BuyItem OnRep_BuyItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_SellItem);
    FOnRep_SellItem OnRep_SellItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_UseItem);
    FOnRep_UseItem OnRep_UseItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_EquipGear);
    FOnRep_EquipGear OnRep_EquipGear;

    DECLARE_MULTICAST_DELEGATE(FOnRep_UnequipGear);
    FOnRep_UnequipGear OnRep_UnequipGear;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

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

protected:
    Protocol::PlayerInfo* _PlayerInfo;
    Protocol::StatInfo* _StatInfo;

    UPROPERTY()
    TObjectPtr<class UInventory> InventoryHelper;

    UPROPERTY()
    TObjectPtr<class UEquippedGear> EquippedGearHelper;

	const float MOVE_PACKET_SEND_DELAY = 0.2f;
	float MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

	// Cache
	FVector2D DesiredInput;
	FVector DesiredMoveDirection;
	float DesiredYaw;

	// Dirty Flag
	FVector2D LastDesiredInput;
};
