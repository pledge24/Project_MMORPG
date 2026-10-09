#pragma once
#include "Protocol/Enum.pb.h"

/*--------------------------------------------------------------
    기획 데이터 템플릿

    부팅할 때 GamedataParser가 기획표(Json)를 검증해 이 구조체로 바꾼다.
    게임 코드는 Json을 읽지 않고 Gamedata::Find*로 이 구조체만 읽는다.
    부팅 뒤에는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
---------------------------------------------------------------*/

/** 기획표의 좌표. */
struct TemplatePos
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

/** 아이템 표(S_Item.json)의 한 행. */
struct ItemTemplate
{
    int32 templateId = 0;
    Protocol::ItemType itemType = Protocol::ITEM_TYPE_NONE;
    /** 장비만 값이 있다. 아이템 세부 종류(itemSubtype)로 정한 착용 부위다. */
    optional<Protocol::GearType> gearType;

    int64 buyPrice = 0;
    int64 sellPrice = 0;
    bool sellable = false;
    /** 1 이상이다. */
    int32 maxStack = 1;
    /** 같은 템플릿을 다시 쓰기까지의 대기(ms). 기획표의 초 단위 값이 0 이하면 0이다. */
    uint64 cooldownMs = 0;

    //~ 장비가 올려 주는 스탯. 표에 없으면 0이다.
    int32 hp = 0;
    int32 mp = 0;
    int32 physicalAttack = 0;
    int32 magicalAttack = 0;

    //~ 소모품이 회복하는 비율(최대치 기준). 표에 없으면 0이다.
    double hpRestoreRatio = 0.0;
    double mpRestoreRatio = 0.0;
};

/** 몬스터 표(S_Monster.json)의 한 행. 보상의 최솟값은 최댓값보다 크지 않다. */
struct MonsterTemplate
{
    int32 templateId = 0;
    int32 maxHp = 0;
    int32 baseAttack = 0;
    float movementSpeed = 0.f;
    bool isTargeting = false;

    int64 minExp = 0;
    int64 maxExp = 0;
    int64 minGold = 0;
    int64 maxGold = 0;

    /** 공격 간격(초). */
    float attackInterval = 0.f;
    float tryAttackRange = 0.f;
    float detectionRange = 0.f;
    float chasingMaxRange = 0.f;
};

/** 직업별 레벨 표의 한 행. 증가량은 이전 레벨에서 이 레벨로 오를 때 더하는 값이다. */
struct LevelTemplate
{
    int32 level = 0;
    int32 maxHp = 0;
    int32 maxMp = 0;
    int32 physicalAttack = 0;
    int32 magicalAttack = 0;
    /** 이 레벨에서 다음 레벨로 가는 데 필요한 경험치. */
    int64 expRequirement = 0;

    int64 maxHpIncrement = 0;
    int64 maxMpIncrement = 0;
    int64 paIncrement = 0;
    int64 maIncrement = 0;
};

/**
 * 한 직업의 레벨 표. 행은 레벨 1부터 빠짐없이 이어진다.
 * 마지막 행의 레벨이 그 직업의 최대 레벨이다.
 */
class ClassLevelTable
{
public:
    ClassLevelTable() = default;
    /** rows[i]의 레벨이 i + 1이어야 한다. GamedataParser가 확인한다. */
    explicit ClassLevelTable(vector<LevelTemplate> rows) : _rows(std::move(rows)) {}

    /** 표 밖의 레벨이면 nullptr. */
    const LevelTemplate* Find(int32 level) const
    {
        if (level < 1 || level > GetMaxLevel())
            return nullptr;

        return &_rows[level - 1];
    }

    /** 행이 없으면 0이다. */
    int32 GetMaxLevel() const { return static_cast<int32>(_rows.size()); }

private:
    vector<LevelTemplate> _rows;
};

/** 맵 표의 포털 하나. 들어서면 목적지 룸의 목적지 좌표로 옮긴다. */
struct PortalTemplate
{
    int32 portalId = 0;
    int32 dstRoomId = 0;
    TemplatePos dstPos;
    float dstYaw = 0.f;
};

/** 맵 표(S_Map.json)의 한 행. 룸 하나가 이 행 하나로 만들어진다. */
struct MapTemplate
{
    /** 룸 번호로 쓴다. */
    int32 templateId = 0;
    TemplatePos center;
    /** X 방향 반경. */
    float depthHalfExtent = 0.f;
    /** Y 방향 반경. */
    float widthHalfExtent = 0.f;
    /** 리스폰 지점이 있는 맵만 값이 있다. 지금은 마을 하나뿐이다. */
    optional<TemplatePos> respawnPoint;
    vector<PortalTemplate> portals;

    /** 이 룸에 스폰할 몬스터 템플릿. 비어 있으면 몬스터를 스폰하지 않는다. */
    vector<int32> monsterIds;
    /** 표에 없으면 0이다. */
    int32 maxMonsterCount = 0;
    /** 표에 없으면 0이다. */
    float monsterRespawnTime = 0.f;

    /** 이 맵에 없는 포털이면 nullptr. */
    const PortalTemplate* FindPortal(int32 portalId) const
    {
        for (const PortalTemplate& portal : portals)
        {
            if (portal.portalId == portalId)
                return &portal;
        }

        return nullptr;
    }
};

/** 부팅 때 만들어 Gamedata에 한 번 설치하는 표 전체. 테스트는 이것을 직접 채워 설치한다. */
struct GamedataTables
{
    unordered_map<int32, ItemTemplate> items;
    unordered_map<int32, MonsterTemplate> monsters;
    /** 룸을 번호 순서로 만들도록 정렬된 맵을 쓴다. */
    map<int32, MapTemplate> maps;
    /** 직업(Protocol::CharacterClass) → 레벨 표. 표가 없는 직업은 캐릭터를 만들 수 없다. */
    unordered_map<int32, ClassLevelTable> classLevelTables;
    /** 사망한 플레이어가 돌아가는 마을 룸. 리스폰 지점이 있는 유일한 맵이다. */
    int32 townRoomId = 0;
};
