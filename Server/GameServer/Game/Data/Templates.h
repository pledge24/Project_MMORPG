#pragma once
#include "Protocol/Common.pb.h"
#include "Protocol/Item.pb.h"
#include "Game/Data/Generated/DesignData.h"
#include "Game/Data/CreatureCombat.h"
#include "Game/Data/Placement.h"

/*--------------------------------------------------------------
    기획 데이터 템플릿

    게임 코드는 기획표(Json)를 직접 읽지 않고 이 구조체만 읽는다.
    설치한 뒤에는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
---------------------------------------------------------------*/

/** Gamedata에 한 번에 설치하는 기획 데이터 표 전체. */
struct GamedataTables
{
    DesignData::ItemTable items;
    /** 장비 아이템의 착용 부위와 스탯. 장비마다 행이 하나다(DataBuilder가 검사한다). */
    DesignData::ItemEquipStatTable itemEquipStats;
    /** 소모품의 사용 효과. 소모품마다 행이 하나다(DataBuilder가 검사한다). */
    DesignData::ItemConsumeEffectTable itemConsumeEffects;
    /** 기본 보상의 최솟값은 최댓값보다 크지 않다(DataBuilder가 검사한다). */
    DesignData::MonsterTable monsters;
    /** 몬스터의 드롭 그룹. 드롭 항목은 그룹의 Entries에 있다. */
    DesignData::DropGroupTable dropGroups;
    /** 시작 맵과 플레이어의 마지막 위치가 가리키는 맵. */
    DesignData::MapTable maps;
    /**
     * 맵 안의 지역. 룸 안에서 규칙이 다른 공간이고, 룸은 지역마다 몬스터를 스폰한다. 포털은 지역의 Portals에 있다.
     * 포털의 도착 지역과 나오는 몬스터는 다른 표에 있고, 반경은 0보다 크다(DataBuilder가 검사한다).
     */
    DesignData::AreaTable areas;
    /** 레벨마다 다음 레벨까지 필요한 경험치. 1부터 이어지고 마지막 레벨이 최대 레벨이다(DataBuilder가 검사한다). */
    DesignData::LevelTable levels;
    /**
     * 직업과 레벨마다의 능력치. 표에 있는 직업은 모든 레벨에 행이 있다(DataBuilder가 검사한다).
     * 로비가 생성할 수 있는 직업은 모두 표에 있다(기동할 때 대조한다).
     */
    DesignData::ClassLevelStatTable classLevelStats;
    /** 사망한 플레이어가 돌아가는 마을. 리스폰 지점이 있는 유일한 지역이다(DataBuilder가 검사한다). */
    int32 townAreaId = 0;

    DesignData::CurrencyTable currencies;
    /** 재화 종류가 GOLD인 재화. DataBuilder가 하나뿐임을 검사한다. */
    int32 goldCurrencyId = 0;
    /** 새 캐릭터가 첫 게임 입장에서 갖는 값. 시작 맵은 맵 표에 있고 시작 골드는 0 이상이다(DataBuilder가 검사한다). */
    DesignData::ProgressConfig initialProgress;

    /** 상점의 판매 목록. 목록은 비어 있지 않고 아이템 표의 아이템이다(DataBuilder가 검사한다). */
    DesignData::ShopTable shops;
    /** NPC가 여는 상점. 상점은 상점 표에 있다(DataBuilder가 검사한다). */
    DesignData::NpcTable npcs;
    /** NPC 상호작용의 사거리. 0보다 크다(DataBuilder가 검사한다). */
    DesignData::InteractionConfig interaction;

    // 배치 데이터. 맵, 지역, 포털마다 하나씩 있고 마을에는 리스폰 지점이 있다(기동할 때 대조하고, 위치는 추출이 검사한다).
    map<int32, MapPlacement> mapPlacements;
    map<int32, AreaPlacement> areaPlacements;
    map<int32, PortalPlacement> portalPlacements;
    /** 지역 번호 → 그 지역의 리스폰 지점. */
    map<int32, RespawnPointPlacement> respawnPoints;
    /** NPC 번호 → 그 NPC의 마커. NPC 표의 NPC마다 하나다. */
    map<int32, NpcPlacement> npcPlacements;

    // 크리처 판정 데이터. 몬스터 표의 몬스터와 직업별 능력치 표의 직업마다 하나씩 있다(기동할 때 대조하고, 타이밍은 추출이 검사한다).
    map<int32, CreatureCombat> monsterCombats;
    map<Protocol::CharacterClass, CreatureCombat> classCombats;
    /** 루트 모션 곡선의 초당 표본 수. */
    int32 rootMotionSampleRate = 0;
};
