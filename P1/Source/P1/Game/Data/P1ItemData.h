#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Enum.pb.h"
#include "P1ItemData.generated.h"

USTRUCT(BlueprintType)
struct FP1ItemData : public FTableRowBase
{
    GENERATED_BODY()

    /** ItemType 문자열을 열거형으로 바꾼다. 종류가 아니거나 비었으면 ITEM_TYPE_NONE이다. */
    Protocol::ItemType GetItemType() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    /** 아이템 종류다. 기획 원본의 값(GEAR, CONSUMABLE, MISCELLANEOUS)을 그대로 담는다. 판정에는 GetItemType을 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemType;

    /** 아이템 분류다(helmet, sword, potion 등). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemSubtype;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelRequirement = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ClassRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BuyPrice = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SellPrice = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Sellable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStack = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Cooldown = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    //~ Consumable Field

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HpRestore = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MpRestore = 0.f;
    
    //~ Gear Field
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PhysicalAttack = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagicalAttack = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Hp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Mp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HpRegenerate = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MpRegenerate = 0;
};