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
    int32 Name = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TemplateId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MonsterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 MaxHp = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 BaseAttack = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftClassPtr<class AMonster> MonsterClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed = 0.f;
};