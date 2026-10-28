#include "Game/Entities/P1Monster.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Game/Combat/P1AttackSystemComponent.h"
#include "Game/Data/P1MonsterAssetData.h"
#include "Materials/MaterialInterface.h"
#include "Utils/LogCategory.h"

AP1Monster::AP1Monster()
{
    // 몬스터는 블루프린트 컴포넌트 없이 이 컴포넌트에 표현 데이터의 공격 몽타주를 넣는다.
    AttackSystemComponent = CreateDefaultSubobject<UP1AttackSystemComponent>(TEXT("AttackSystem"));
}

void AP1Monster::Initialize(const Protocol::EntityInfo& EntityInfo)
{
    Super::Initialize(EntityInfo);

    if (EntityInfo.has_monster_info() == false)
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("몬스터 정보가 없는채로 몬스터 초기화 시도함"));
        return;
    }

    _MonsterInfo.CopyFrom(EntityInfo.monster_info());

    TemplateId = _MonsterInfo.template_id();
    CurHp = _MonsterInfo.hp();
}

void AP1Monster::S_Die()
{
    Super::S_Die();

    // 서버는 사망한 몬스터에 S_DESPAWN을 보내지 않으므로 클라이언트가 스스로 지워야 한다. 지우지 않으면 시체가 남는다.
    // 사망 애니메이션은 ABP가 OnDie를 받아 재생한다. 그 시간을 준 뒤 지운다.
    // 그사이 룸 이동으로 먼저 지워지면 AActor::EndPlay가 이 타이머를 함께 지운다.
    GetWorldTimerManager().SetTimer(DespawnTimerHandle, this, &AP1Monster::DespawnAfterDeath, DESPAWN_DELAY_SECONDS, false);
}

void AP1Monster::DespawnAfterDeath()
{
    // Destroy만 하면 매니저의 등록이 남는다. 등록을 가진 매니저가 구독해서 등록과 액터를 함께 지운다.
    OnDespawnReady.Broadcast(this);
}

void AP1Monster::ApplyAssetData(const FP1MonsterAssetData& AssetData)
{
    if (USkeletalMeshComponent* MeshComponent = GetMesh())
    {
        MeshComponent->SetSkeletalMeshAsset(AssetData.SkeletalMesh.LoadSynchronous());
        MeshComponent->SetAnimInstanceClass(AssetData.AnimClass.LoadSynchronous());

        // 빈 칸은 메시의 머티리얼을 그대로 둔다.
        for (int32 Index = 0; Index < AssetData.Materials.Num(); Index++)
        {
            if (UMaterialInterface* Material = AssetData.Materials[Index].LoadSynchronous())
                MeshComponent->SetMaterial(Index, Material);
        }

        MeshComponent->SetRelativeLocation(AssetData.MeshOffset);
        MeshComponent->SetRelativeScale3D(AssetData.MeshScale);
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
        Capsule->SetCapsuleSize(AssetData.CapsuleRadius, AssetData.CapsuleHalfHeight);

    if (AttackSystemComponent != nullptr)
    {
        TArray<UAnimMontage*> Montages;
        for (const TSoftObjectPtr<UAnimMontage>& Montage : AssetData.AttackMontages)
        {
            if (UAnimMontage* Loaded = Montage.LoadSynchronous())
                Montages.Add(Loaded);
        }
        AttackSystemComponent->SetNormalAttackMontages(Montages);
    }
}

void AP1Monster::SetDefaultMonsterData(const FP1MonsterRow& InMonsterData)
{
    MonsterData = InMonsterData;

    CreatureName = FText::FromString(MonsterData.Name);
    TemplateId = MonsterData.Id;
    if (CurHp < 0)
        CurHp = MonsterData.MaxHp;
}

