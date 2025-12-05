#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterData.generated.h"

/*------------------
     MonsterData
-------------------*/

USTRUCT(BlueprintType)
struct FMonsterData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MonsterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 MaxHp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 BaseAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftClassPtr<class AMonster> MonsterClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed;
};