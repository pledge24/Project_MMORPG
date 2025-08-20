#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ItemData.generated.h"

/*-----------------
      ItemData
-----------------*/

USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemSubType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EquipmentSlotId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ClassRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BuyPrice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SellPrice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BaseAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BaseMagicAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HpBonus;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MpBonus;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HpRestore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MpRestore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool QuestItem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 VendorNpcId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Equippable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Sellable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Consumable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Cooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;
};