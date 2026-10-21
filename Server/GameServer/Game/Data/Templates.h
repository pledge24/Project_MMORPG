#pragma once
#include "Protocol/Enum.pb.h"

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

/** 아이템 표(S_Item.json)의 한 행. */
struct ItemTemplate
{
    int32 templateId = 0;
    Protocol::ItemType itemType = Protocol::ITEM_TYPE_NONE;
    /** 착용 부위. 장비만 값이 있다. */
    optional<Protocol::GearType> gearType;

    //~ 착용 조건. 장착할 때 서버가 판정한다.
    int32 levelRequirement = 0;
    /** 비어 있으면 모든 직업이 쓴다. 기획표의 "all"이다. */
    optional<Protocol::CharacterClass> classRequirement;

    int64 buyPrice = 0;
    int64 sellPrice = 0;
    bool sellable = false;
    /** 1 이상이다. */
    int32 maxStack = 1;
    /** 같은 템플릿을 다시 쓰기까지의 대기(ms). 0이면 대기가 없다. */
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
 * 한 직업의 레벨 표.
 *
 * 레벨마다 능력치와 필요 경험치를 담고, 그 직업의 최대 레벨을 정한다.
 */
class ClassLevelTable
{
public:
    ClassLevelTable() = default;
    /** rows[i]의 레벨이 i + 1이어야 한다. 여기서는 확인하지 않는다. */
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

/** 재화 표의 재화 종류. 서버가 종류로 찾아 쓰는 재화다. 기획 원본은 열거자 이름(GOLD)을 적는다. */
enum class CurrencyType : uint8
{
    GOLD = 1,
};

/** 재화 표(S_Currency.json)의 한 행. */
struct CurrencyTemplate
{
    /** DB가 재화를 가리키는 번호(CurrencyId)로 쓴다. */
    int32 templateId = 0;
    CurrencyType currencyType = CurrencyType::GOLD;
};

/** 캐릭터 생성 표(S_Character_Creation.json)의 유일한 행. 새 캐릭터와 새 계정이 처음 갖는 값이다. */
struct CharacterCreationTemplate
{
    /** 시작 맵. 맵 표의 mapId다. */
    int32 startMapId = 0;
    TemplatePos startPos;
    float startYaw = 0.f;
    /** 0 이상이다. */
    int64 startGold = 0;
    /** 1 이상 255 이하다. */
    int32 defaultCharacterSlotCount = 0;
};

/** Gamedata에 한 번에 설치하는 기획 데이터 표 전체. */
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

    unordered_map<int32, CurrencyTemplate> currencies;
    /** 재화 종류가 GOLD인 유일한 재화. */
    int32 goldCurrencyId = 0;
    CharacterCreationTemplate characterCreation;
    /** 아이템 종류(Protocol::ItemType) → 인벤토리에서 그 종류 구역의 칸 수. NONE을 뺀 모든 종류가 있다. */
    unordered_map<int32, int32> inventorySlotCounts;
};
