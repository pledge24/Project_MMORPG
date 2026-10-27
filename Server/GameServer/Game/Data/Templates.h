#pragma once
#include "Protocol/Common.pb.h"
#include "Protocol/Item.pb.h"
#include "Game/Data/Generated/DesignData.h"

/*--------------------------------------------------------------
    기획 데이터 템플릿

    게임 코드는 기획표(Json)를 직접 읽지 않고 이 구조체만 읽는다.
    설치한 뒤에는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
---------------------------------------------------------------*/

/** 기획표의 좌표. */
struct TemplatePos
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

/** 맵 표의 포털 하나. 출발 위치에서 맵의 portalRadius 안에 들어서면 목적지 룸의 목적지 좌표로 옮긴다. */
struct PortalTemplate
{
    int32 portalId = 0;
    /** 포털이 놓인 위치. 이 룸의 좌표다. */
    TemplatePos srcPos;
    int32 dstRoomId = 0;
    TemplatePos dstPos;
    float dstYaw = 0.f;
};

/** 맵 표(S_Map.json)의 한 행. 룸 하나가 이 행 하나로 만들어진다. */
struct MapTemplate
{
    /** 룸 번호로 쓴다. */
    int32 templateId = 0;
    /** 이 룸이 속한 맵. 같은 맵의 룸끼리는 포털로, 다른 맵의 룸으로는 맵 간 이동으로 옮긴다. */
    int32 mapId = 0;
    TemplatePos center;
    /** X 방향 반경. */
    float depthHalfExtent = 0.f;
    /** Y 방향 반경. */
    float widthHalfExtent = 0.f;
    /** 리스폰 지점. 마을만 값이 있다. */
    optional<TemplatePos> respawnPoint;
    vector<PortalTemplate> portals;
    /** 이 룸의 포털을 탈 수 있는 거리. 평면(x, y) 거리로 재고 0보다 크다. */
    float portalRadius = 0.f;

    /** 이 룸에 스폰할 몬스터 템플릿. 비어 있으면 몬스터를 스폰하지 않는다. */
    vector<int32> monsterIds;
    /** 표에 없으면 0이다. */
    int32 maxMonsterCount = 0;
};

/** 초기 진행 표(S_Initial_Progress.json)의 유일한 행. 새 캐릭터가 첫 게임 입장에서 갖는 값이다. */
struct InitialProgressTemplate
{
    /** 시작 맵. 맵 표의 mapId다. */
    int32 startMapId = 0;
    TemplatePos startPos;
    float startYaw = 0.f;
    /** 0 이상이다. */
    int64 startGold = 0;
};

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
    /** 룸을 번호 순서로 만들도록 정렬된 맵을 쓴다. */
    map<int32, MapTemplate> maps;
    /** 레벨마다 다음 레벨까지 필요한 경험치. 1부터 이어지고 마지막 레벨이 최대 레벨이다(DataBuilder가 검사한다). */
    DesignData::LevelTable levels;
    /**
     * 직업과 레벨마다의 능력치. 표에 있는 직업은 모든 레벨에 행이 있다(DataBuilder가 검사한다).
     * 로비가 생성할 수 있는 직업은 모두 표에 있다(기동할 때 대조한다).
     */
    DesignData::ClassLevelStatTable classLevelStats;
    /** 사망한 플레이어가 돌아가는 마을 룸. 리스폰 지점이 있는 유일한 맵이다. */
    int32 townRoomId = 0;

    DesignData::CurrencyTable currencies;
    /** 재화 종류가 GOLD인 재화. DataBuilder가 하나뿐임을 검사한다. */
    int32 goldCurrencyId = 0;
    InitialProgressTemplate initialProgress;
    /** 아이템 종류(Protocol::ItemType) → 인벤토리에서 그 종류 구역의 칸 수. NONE을 뺀 모든 종류가 있다. */
    unordered_map<int32, int32> inventorySlotCounts;
    /** 플레이어 한 명의 퀵슬롯 개수. 1 이상 256 이하다. */
    int32 quickSlotCount = 0;
};
