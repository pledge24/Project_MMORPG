#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1MonsterAssetData.generated.h"

class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class USkeletalMesh;

/** 몬스터의 표현 데이터. 행 이름은 TemplateId이고, 기획 원본에서 오지 않고 에디터에서만 편집한다. */
USTRUCT(BlueprintType)
struct P1_API FP1MonsterAssetData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftClassPtr<UAnimInstance> AnimClass;

    /** 머티리얼 슬롯 순서대로 덮어쓴다. 빈 칸과 넘치는 슬롯은 메시의 머티리얼을 그대로 쓴다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TArray<TSoftObjectPtr<UMaterialInterface>> Materials;

    /** 캡슐 중심에서 본 메시의 위치(cm). 발이 캡슐 바닥에 닿게 맞춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    FVector MeshOffset = FVector(0.f, 0.f, -88.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    FVector MeshScale = FVector::OneVector;

    /** 콤보 순번 1부터 차례로 재생하는 일반 공격 몽타주. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TArray<TSoftObjectPtr<UAnimMontage>> AttackMontages;

    /** 캡슐 반지름(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CapsuleRadius = 34.f;

    /** 캡슐 반높이(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CapsuleHalfHeight = 88.f;
};
