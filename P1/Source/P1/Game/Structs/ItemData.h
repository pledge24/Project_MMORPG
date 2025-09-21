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
    int32 LevelRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ClassRequirement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BuyPrice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SellPrice;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool Sellable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Cooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;

    /*---------------------------
           Conumption Field
    ----------------------------*/

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HpRestore;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MpRestore;
    
    /*---------------------------
              Gear Field
    ----------------------------*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PhysicalAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MagicalAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Hp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Mp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HpRegenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MpRegenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> StaticMesh;
};