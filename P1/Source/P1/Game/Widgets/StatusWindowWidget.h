// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlotWidget.h"
#include "StatusWindowWidget.generated.h"

class AP1MyPlayer;
class UTextBlock;
class UMyPlayerData;

/**
 * 
 */
UCLASS()
class P1_API UStatusWindowWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    void UpdateSlotWidget(const Protocol::Slot& Slot_);
    void UpdateAllStat(UMyPlayerData* MyPlayerData);

    void UpdateMaxHp(int32 Value);
    void UpdateMaxMp(int32 Value);
    void UpdatePhysicalAttack(int32 Value);
    void UpdateMagicalAttack(int32 Value);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendUnequipPacket(USlotWidget* Slot_);

    // 장착 중인 장비
    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category="EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Helmet;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Chest;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Arms;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Legs;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Boots;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Weapon;

    // 상세 스텟 정보
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Details_MaxHp;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Details_MaxMp;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Details_Physical_Attack;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Details_Magical_Attack;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
