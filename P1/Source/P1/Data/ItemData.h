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
    int32 Name = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemSubType;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;

    /*---------------------------
           Conumption Field
    ----------------------------*/

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HpRestore = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MpRestore = 0.f;
    
    /*---------------------------
              Gear Field
    ----------------------------*/
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> StaticMesh;
};