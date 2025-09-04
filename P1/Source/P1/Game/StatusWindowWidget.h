// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlotWidget.h"
#include "StatusWindowWidget.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class P1_API UStatusWindowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void UpdateSlot(const Protocol::Slot& _Slot);
    void UpdateAllStat(const Protocol::StatInfo& _StatInfo);

    void UpdateMaxHp(int32 Value);
    void UpdateMaxMp(int32 Value);
    void UpdatePhysicalAttack(int32 Value);
    void UpdateMagicalAttack(int32 Value);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendUnequipPacket(USlotWidget* _Slot);

    // 장착 중인 장비
    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category="EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Helmet;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Chest;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<USlotWidget> Equipped_Gloves;

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
};
