#pragma once
#include "Game/Entities/Creature.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Entities/CombatStats.h"
#include "Game/Items/ItemTypes.h"
#include "Game/Items/ItemOutcome.h"
#include "Game/QuickSlots/QuickSlotTypes.h"

class GameSession;
class Room;
class InventoryComponent;
class EquipmentComponent;
class QuickSlotComponent;
struct RoomEnterData;
struct TownRespawn;
struct LevelExp;

//~ 요청 처리 결과

/** 클라이언트가 가리킨 칸과 그 칸에 있다고 본 아이템의 uid. 모든 값이 클라이언트 값이다. */
struct ItemSlotRequest
{
    SlotAddress address;
    /** 0이면 칸에 아이템이 없다고 본 것이다. */
    int64 itemUid = 0;
};

/** 구매 결과. */
struct BuyItemResult
{
    vector<SlotChange> changedSlots;
    int64 gold = 0;
};

/** 판매 결과. 판매는 칸 하나만 바꾼다. */
struct SellItemResult
{
    vector<SlotChange> changedSlots;
    int64 gold = 0;
};

/** 소모품 사용 결과. */
struct UseItemResult
{
    vector<SlotChange> changedSlots;
    RepeatedPtrField<Protocol::Stat> changedStats;
};

/** 장비 착용과 해제의 결과. 바뀐 칸은 착용 장비의 칸이 먼저다. */
struct GearChangeResult
{
    /** 외형이 바뀐 부위와 그 부위의 아이템. 해제면 templateId가 0이다. */
    Protocol::GearPart gearPart = Protocol::GEAR_PART_NONE;
    int32 templateId = 0;
    vector<SlotChange> changedSlots;
    RepeatedPtrField<Protocol::Stat> changedStats;
};

/** 퀵슬롯 등록 결과. 교환이면 두 퀵슬롯이 바뀐다. */
struct SetQuickSlotResult
{
    vector<QuickSlot> changedQuickSlots;
};

/** 리스폰 결과. */
struct RespawnResult
{
    RepeatedPtrField<Protocol::Stat> updatedStats;
};

/** 보상으로 오른 레벨. */
struct LevelUpResult
{
    /** 오른 뒤의 레벨. */
    int32 level = 0;
    RepeatedPtrField<Protocol::Stat> changedStats;
};

/** 보상을 적용한 결과. */
struct RewardResult
{
    /** 보상을 더한 뒤 지금 레벨 안의 경험치. */
    int64 exp = 0;
    /** 보상을 더한 뒤의 골드. */
    int64 gold = 0;
    /** 레벨이 오르지 않았으면 비어 있다. */
    optional<LevelUpResult> levelUp;
};

/** 플레이어의 스폰 매개변수. */
struct PlayerSpawnParams : public Creature::SpawnParams
{
    /** 비어 있으면 세션에 연결하지 않는다. 운영 코드는 언제나 넘기고, 테스트만 비워 둔다. */
    GameSessionRef session;
    int64 userId = 0;
    /** 불러온 진행(nullptr 가능). Init 안에서만 읽으므로 Init이 끝날 때까지만 살아 있으면 된다. */
    const PlayerProgress* progress = nullptr;
};

/**
 * 접속한 캐릭터 하나.
 *
 * 캐릭터의 진행(레벨, 스탯, 소지품)을 들고, 플레이어의 요청을 받아 그 진행을 바꾼다.
 * 접속이 끊기면 저장할 진행의 사본을 내준다.
 */
class Player : public Creature
{
public:
    using SpawnParams = PlayerSpawnParams;

	Player();
	virtual ~Player();

protected:
    friend class EntityFactory;

    /**
     * 세션의 _player는 바꾸지 않으므로 세션에 등록하는 일은 호출자가 한다.
     * progress가 있으면 그 진행으로 채우고 검증까지 한다. 검증에 실패하면 false.
     */
    bool Init(const SpawnParams& params);

    //~ Begin Entity Interface
protected:
    virtual void Start() override;
    virtual void Tick(float deltaTime) override;
    //~ End Entity Interface

    //~ 불러오기
public:
    /**
     * 플레이어 정보를 모두 채운 뒤 부른다. 언리얼의 FinishSpawning에 대응한다.
     * 불러온 현재 HP나 MP가 계산한 최대치를 넘으면 false.
     */
    bool OnLoaded();

private:
    /**
     * 불러온 진행을 플레이어에 쓴다. 스탯은 검증하지 않으므로 뒤이어 OnLoaded를 부른다.
     * 넣을 수 없는 슬롯이 하나라도 있으면 사유를 로그에 남기고 false.
     */
    bool ApplyProgress(const PlayerProgress& progress);
    /** 불러온 현재 HP나 MP가 계산한 최대치를 넘으면 사유를 로그에 남기고 false. */
    bool ValidateLoadedStat();

    //~ 요청 처리
public:
    /** 클라이언트 값을 넘겨도 된다. 결과의 gold는 남은 골드다. */
    ItemOutcome<BuyItemResult> ProcessBuyItem(int32 templateId, int32 count = 1);
    /** 클라이언트 값을 넘겨도 된다. 결과의 gold는 남은 골드다. */
    ItemOutcome<SellItemResult> ProcessSellItem(const ItemSlotRequest& request, int32 count = 1);
    /** 소모품을 쓴다. nowMs는 재사용 대기 판정에 쓰는 현재 시각(ms, GetTickCount64)이다. */
    ItemOutcome<UseItemResult> ProcessUseItem(const ItemSlotRequest& request, uint64 nowMs);
    /** 결과의 gearPart는 요청한 인벤토리 칸이 아니라 장착된 장비 부위다. */
    ItemOutcome<GearChangeResult> ProcessEquipGear(const ItemSlotRequest& request);
    /** 결과의 templateId는 0이다. */
    ItemOutcome<GearChangeResult> ProcessUnequipGear(const ItemSlotRequest& request);
    /**
     * slotIndex 퀵슬롯에 ref를 등록하고, ref가 비었으면 해제한다. 클라이언트 값을 넘겨도 된다.
     * 소지 여부는 보지 않는다.
     */
    ItemOutcome<SetQuickSlotResult> ProcessSetQuickSlot(int32 slotIndex, const optional<QuickSlotRef>& ref);
    /**
     * slotIndex 퀵슬롯에 등록된 소모품을 소모품 구역에서 칸 번호가 가장 작은 칸부터 쓴다. 클라이언트 값을 넘겨도 된다.
     * 그 밖의 거절 사유는 ProcessUseItem과 같다.
     */
    ItemOutcome<UseItemResult> ProcessUseQuickSlot(int32 slotIndex, uint64 nowMs);
    /**
     * respawnPos에서 되살린다. 소속 룸을 보지 않으므로 룸에서 뺀 뒤에도 부를 수 있다.
     * 마을 리스폰만 경험치를 잃고 HP를 절반만 채운다.
     */
    RespawnResult ProcessRespawn(Protocol::RespawnType type, const Protocol::PosInfo& respawnPos);

    //~ 이벤트
public:
    virtual void OnHit(EntityRef attacker, const AttackInfo& attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    //~ 맵과 룸 이동
public:
    /**
     * room의 맵을 클라이언트가 다 열 때까지 기다리게 한다. 맵과 위치를 목적지의 것으로 바꾸고 소속 룸을 room으로 정한다.
     * 룸에는 넣지 않는다. 세션에 등록하기 전이나, 플레이어를 뺀 룸의 큐 위에서 부른다.
     */
    void BeginMapLoad(const RoomRef& room, const Protocol::PosInfo& pos);
    /** roomId 룸에서 맵 로드를 기다리고 있었으면 기다림을 끝내고 true. 소속 룸 큐 위에서 부른다. */
    bool FinishMapLoad(int32 roomId);
    /** 입장하는 룸의 큐 위에서 부른다. enterPos가 없으면 룸 중심에 둔다. */
    void OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos);
    /**
     * 위치를 posInfo로 바꾼다. 이동을 검증하지 않으므로 판정을 통과한 요청만 넘긴다.
     * now는 다음 이동 판정의 기준 시각이 된다. 소속 룸 큐 위에서 부른다.
     */
    void ApplyMove(const Protocol::PosInfo& posInfo, uint64 now);
    /** 서버가 위치를 정했거나 이동을 받아들인 마지막 시각(ms, GetTickCount64). */
    uint64 GetLastPositionTick() const { return _lastPositionTick; }
    /** 지금 맵 또는 들어가는 맵. 맵 표의 mapId다. */
    int32 GetMapId() const { return _mapId; }
    /** 마지막으로 들어간 룸. 룸에 들어간 적이 없으면 -1. */
    int32 GetRoomId() const { return _roomId; }

private:
    int32 _mapId = 0;
    int32 _roomId = -1;
    /** 맵 로드를 기다리는 룸. 기다리지 않으면 -1. 소속 룸 큐만 읽고 쓴다. */
    int32 _mapLoadRoomId = -1;
    /** 이동 판정은 이 시각부터 흐른 시간으로 허용 거리를 정한다. 소속 룸 큐만 읽고 쓴다. */
    uint64 _lastPositionTick = 0;

    //~ 레벨과 보상
public:
    /** 보상을 적용하고 그 결과를 돌려준다. 최대 레벨에서는 경험치를 버린다. */
    RewardResult OnGetReward(const Reward& reward);
    /** 레벨 표의 마지막 레벨이면 true. 직업별 능력치 표에 없는 직업도 true. */
    bool IsMaxLevel() const;

private:
    /** 레벨을 하나 올리고 두 레벨의 능력치 차이를 스탯에 더한다. 최대 레벨이면 아무것도 하지 않는다. */
    void OnLevelUp();
    /**
     * 누적 경험치를 바꾸고, 클라이언트에 보낼 레벨 안의 경험치와 그 레벨의 요구치를 스탯에 쓴다.
     * 레벨은 바꾸지 않는다. 돌려받은 레벨에 맞추는 일은 호출자가 한다.
     */
    LevelExp SetTotalExp(int64 totalExp);

    /** DB에 저장하는 누적 경험치. 스탯의 경험치는 이 값에서 계산한 레벨 안의 값이다. */
    int64 _totalExp = 0;

    //~ 스탯
private:
    /**
     * 레벨 표의 기본 스탯에 착용 장비의 증감량을 더한 최종 스탯. 최종 스탯은 이 함수로만 계산한다.
     * 레벨 표에 없는 직업이나 레벨이면 nullopt.
     */
    optional<CombatStats> CalculateFinalStat();
    /**
     * 최종 스탯을 다시 계산해 쓰고, 값이 바뀐 스탯을 updatedStats에 싣는다(nullptr 가능).
     * 계산할 수 없으면 아무것도 바꾸지 않는다.
     */
    void RefreshFinalStat(OUT RepeatedPtrField<Protocol::Stat>* updatedStats);

    //~ 소지품
public:
    // 읽기만 연다. 소지품과 퀵슬롯을 바꾸는 길은 요청 처리(Process*)와 불러오기뿐이다.
    int64 GetGold() const { return _gold; }
    const InventoryComponent& GetInventory() const { return *_inventory; }
    const EquipmentComponent& GetEquipment() const { return *_equipment; }
    const QuickSlotComponent& GetQuickSlots() const { return *_quickSlots; }

private:
    /** 다른 플레이어에게 보일 장비 외형 요약을 다시 쓴다. */
    void RefreshEquippedGearSummary();
    /** 착용 장비와 인벤토리에서 바뀐 칸을 그 순서로 꺼낸다. */
    vector<SlotChange> TakeItemChanges();

    int64 _gold = 0;
    InventoryComponentRef _inventory;
    EquipmentComponentRef _equipment;
    QuickSlotComponentRef _quickSlots;

    //~ 세션
public:
    /** 세션이 끊겨 사라졌거나 세션 없이 만든 플레이어(테스트)면 nullptr. */
    GameSessionRef GetSession() const { return _session.lock(); }
    /** 세션이 사라진 뒤에도 읽을 수 있는 계정 번호. Init 뒤로는 바뀌지 않는다. */
    int64 GetUserId() const { return _userId; }

private:
	weak_ptr<GameSession> _session;
    int64 _userId = 0;

    //~ 접속 종료
public:
    /** 접속과 룸 입장의 진행. 접속 종료와 첫 룸 입장 중 어느 쪽이 먼저였는지를 한 값으로 정한다. */
    enum class Presence : uint8
    {
        LOADED,           // 불러와 세션에 등록했고 룸에 들어간 적이 없다
        IN_ROOM,          // 룸에 들어간 적이 있다. 룸 이동 중이어도 이 값이다
        LEFT_BEFORE_ROOM, // 룸에 들어가기 전에 끊겼다. 바뀐 것이 없으므로 저장하지 않는다
        LEFT_IN_ROOM,     // 룸에 들어간 뒤 끊겼다. 저장한다
        SAVED,            // 룸에 들어간 뒤 끊겼고 접속 종료 저장 잡이 끝났다
    };

    /**
     * 접속 종료나 로비 복귀를 표시하고, 그 전에 룸에 들어간 적이 있으면 true. 세션 스레드가 부른다.
     * true면 소속 룸이 이미 정해져 있다.
     */
    bool MarkDisconnected();
    /**
     * 엔티티를 룸에 넣은 뒤 룸 큐 위에서 부르고, 바꾼 뒤의 값을 받는다. 첫 룸 입장이면 IN_ROOM으로 바꾼다.
     * 부른 쪽은 LEFT_BEFORE_ROOM이면 룸에서 빼기만 하고, LEFT_IN_ROOM이면 퇴장과 저장을 이어 받는다.
     */
    Presence MarkEnteredRoom();
    /**
     * 접속 종료 저장이 끝났음을 표시한다. 저장 잡(DB 스레드)이 저장 대기를 풀기 직전에 부른다.
     * DB 스레드가 살아 있는 Player에서 만져도 되는 것은 이 값 하나뿐이다.
     */
    void MarkSaved() { _presence.store(Presence::SAVED); }
    /** 늦게 온 밀어내기가 그 세션의 접속 종료가 이미 끝났는지 볼 때 읽는다. */
    Presence GetPresence() const { return _presence.load(); }

    /** 저장할 진행의 사본을 만든다. 룸 큐 위에서만 부른다. */
    PlayerSaveData MakeSaveData() const;
    /** 사망한 채 끊긴 플레이어를 저장 직전에 마을 리스폰 상태로 바꾼다. 저장할 맵과 룸 번호도 마을로 바꾼다. */
    void ApplyTownRespawnForSave(const TownRespawn& town);

private:
    atomic<Presence> _presence = Presence::LOADED;

    //~ 공통
public:
    /** 다른 클라이언트에도 가는 공개 정보(이름, 레벨, 장비 외형). */
    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_playerInfo; }
    int64 GetCharacterId() const { return _characterId; }
    Protocol::CharacterClass GetCharacterClass() const { return _characterClass; }

private:
    /** 테스트가 준비 단계에서 레벨, 직업, 골드, 소지품을 직접 채운다. */
    friend struct PlayerTestAccess;

    /** _entityInfo 안의 player_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::PlayerInfo* _playerInfo;
    int64 _characterId = 0;
    Protocol::CharacterClass _characterClass = Protocol::CHARACTER_CLASS_NONE;
};

