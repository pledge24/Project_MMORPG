#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Npc.generated.h"

class UCapsuleComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
struct FP1NpcAssetData;

/**
 * 월드에 서 있는 NPC.
 *
 * 모든 NPC가 이 클래스 하나를 쓰고, NPC마다 다른 외형은 표현 데이터로 입는다.
 * 움직이지 않고 싸우지 않으며, 내 플레이어가 조준하면 초점이 맞았음을 외곽선으로 보인다.
 */
UCLASS()
class P1_API AP1Npc : public AActor
{
    GENERATED_BODY()

public:
    AP1Npc();

    //~ Identity
public:
    /** 서버가 보낸 엔티티 번호와 NPC 번호를 넣는다. FinishSpawning 전에 부른다. */
    void Initialize(int64 InEntityId, int32 InNpcId);

    int64 GetEntityId() const { return EntityId; }
    int32 GetNpcId() const { return NpcId; }

private:
    int64 EntityId = 0;

    /** NPC 표의 번호. */
    UPROPERTY(VisibleAnywhere, Category = "Npc")
    int32 NpcId = 0;

    //~ Appearance
public:
    /**
     * 표현 데이터의 메시, 대기 애니메이션, 캡슐을 입힌다. 가리키는 에셋을 동기 로드한다.
     * 캡슐 크기가 바뀌므로 스폰 위치(캡슐 중심)를 정하기 전에 부른다.
     */
    void ApplyAssetData(const FP1NpcAssetData& AssetData);

    float GetCapsuleHalfHeight() const;

private:
    /** 조준 스윕과 플레이어의 이동을 막는 몸통. 루트다. */
    UPROPERTY(VisibleAnywhere, Category = "Npc")
    TObjectPtr<UCapsuleComponent> Capsule;

    UPROPERTY(VisibleAnywhere, Category = "Npc")
    TObjectPtr<USkeletalMeshComponent> Mesh;

    //~ Focus
public:
    /** 내 플레이어의 조준이 이 NPC에 맞았는지. 맞으면 메시에 외곽선 머티리얼을 덧그린다. */
    void SetFocused(bool bFocused);

    /** 상점을 여는 동안 카메라가 바라볼 곳. 캡슐 위쪽, 대략 얼굴 높이다. */
    FVector GetCameraFocusPoint() const;
};
