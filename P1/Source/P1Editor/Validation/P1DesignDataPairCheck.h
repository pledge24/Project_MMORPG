#pragma once

#include "CoreMinimal.h"

class UDataTable;

/**
 * 기획 데이터 테이블과 표현 데이터 테이블(에셋 테이블)의 짝 검사.
 *
 * 두 테이블은 행 이름(기획 데이터의 키)으로 짝을 맺는다. 수치 테이블의 행마다 에셋 테이블에 행이 있고
 * 필수 참조가 채워져 있는지, 에셋 테이블에만 남은 행이 없는지 확인한다.
 */
class FP1DesignDataPairCheck
{
public:
    /** 짝이 맞지 않는 곳 하나. 경고는 저장을 막지 않는다. */
    struct FIssue
    {
        bool bError = false;
        FString Message;
    };

    /**
     * 아이템 수치 테이블과 아이템 에셋 테이블을 대조한다. 장비 능력치 테이블에 행이 있는 아이템을 장비로 본다.
     * 아이콘은 모든 아이템에, 메시는 장비에만 필수다.
     */
    static TArray<FIssue> CheckItems(const UDataTable& ItemTable, const UDataTable& ItemEquipStatTable, const UDataTable& ItemAssetTable);

    /** 몬스터 수치 테이블과 몬스터 에셋 테이블을 대조한다. 메시와 애니메이션 블루프린트가 필수다. */
    static TArray<FIssue> CheckMonsters(const UDataTable& MonsterTable, const UDataTable& MonsterAssetTable);

    /** NPC 수치 테이블과 NPC 에셋 테이블을 대조한다. 메시가 필수다. */
    static TArray<FIssue> CheckNpcs(const UDataTable& NpcTable, const UDataTable& NpcAssetTable);
};
