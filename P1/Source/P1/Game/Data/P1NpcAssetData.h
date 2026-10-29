#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "P1NpcAssetData.generated.h"

class UAnimationAsset;
class USkeletalMesh;

/** NPC의 표현 데이터. 행 이름은 NPC 번호이고, 기획 원본에서 오지 않고 에디터에서만 편집한다. */
USTRUCT(BlueprintType)
struct P1_API FP1NpcAssetData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    /** 서 있는 동안 반복 재생하는 애니메이션. NPC는 움직이지 않으므로 애니메이션 블루프린트 없이 이것 하나를 튼다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftObjectPtr<UAnimationAsset> IdleAnimation;

    /** 캡슐 중심에서 본 메시의 위치(cm). 발이 캡슐 바닥에 닿게 맞춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    FVector MeshOffset = FVector(0.f, 0.f, -90.f);

    /** 캡슐 기준의 메시 회전. 메시의 앞이 +X가 아니면 맞춘다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    FRotator MeshRotation = FRotator(0.f, -90.f, 0.f);

    /** 조준 판정과 충돌에 쓰는 캡슐 반지름(cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CapsuleRadius = 34.f;

    /** 캡슐 반높이(cm). NPC는 바닥에서 이만큼 올라선 자리에 선다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
    float CapsuleHalfHeight = 90.f;
};
