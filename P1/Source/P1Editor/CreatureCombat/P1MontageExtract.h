#pragma once

#include "CoreMinimal.h"
#include "CreatureCombat/P1CreatureCombatCheck.h"

class UAnimMontage;

/**
 * 액션 몽타주의 섹션 하나에서 크리처 판정 데이터를 읽는 자유 함수.
 *
 * 섹션의 길이, 타격 노티파이의 시점, 콤보 입력 창, 루트 모션 누적 곡선을 꺼낸다.
 */
namespace P1MontageExtract
{
    /**
     * Section 섹션의 값을 OutAction에 채운다(번호와 출처는 채우지 않는다). 몽타주에 그 섹션이 없으면 false.
     * MeshToActor는 재생하는 크리처에 붙은 메시의 상대 트랜스폼이다. 루트 모션을 메시 공간에서 캐릭터 로컬 좌표로 옮기는 데 쓴다.
     */
    P1EDITOR_API bool ExtractSection(const UAnimMontage& Montage, FName Section, const FTransform& MeshToActor, FP1CreatureCombatCheck::FAction& OutAction);
}
