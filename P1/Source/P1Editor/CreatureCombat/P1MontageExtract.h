#pragma once

#include "CoreMinimal.h"
#include "CreatureCombat/P1CreatureCombatCheck.h"

class UAnimMontage;

/**
 * 공격 몽타주 하나에서 크리처 판정 데이터를 읽는 자유 함수.
 *
 * 섹션마다 길이, 타격 노티파이의 시점, 루트 모션 누적 곡선을 꺼낸다.
 */
namespace P1MontageExtract
{
    /**
     * MeshToActor는 크리처에 붙은 메시의 상대 트랜스폼이다. 루트 모션을 메시 공간에서 캐릭터 로컬 좌표로 옮기는 데 쓴다.
     * 몽타주의 섹션을 하나도 읽지 못하면 Sections가 비어 있다.
     */
    P1EDITOR_API FP1CreatureCombatCheck::FAttack Extract(const UAnimMontage& Montage, const FTransform& MeshToActor);
}
