#pragma once
#include "Game/Entities/Creature.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Entities/CombatStats.h"

class GameSession;
class Room;
class InventoryComponent;
class EquipmentComponent;
struct RoomEnterData;

/**
 * 다음 레벨로 오를 때 더할 스탯 증가량. 레벨 표의 다음 행을 미리 읽어 둔 것이다.
 * 다음 행이 없으면 모든 값이 0이다.
 */
struct NextLevelUpData
{
    int32 level = 0;
    int64 maxHpIncrement = 0;
    int64 maxMpIncrement = 0;
    int64 paIncrement = 0;
    int64 maIncrement = 0;
    int64 expRequirement = 0;
};

//~ 요청 처리 결과. 응답 패킷은 핸들러가 이 결과로 만든다.

/** 구매 결과. updatedSlots는 수량이 바뀐 인벤토리 칸이다. */
struct BuyItemResult
{
    RepeatedPtrField<Protocol::Slot> updatedSlots;
    int64 gold = 0;
};

/** 판매 결과. */
struct SellItemResult
{
    Protocol::Slot updatedSlot;
    int64 gold = 0;
};

/** 소모품 사용 결과. updatedStats는 회복으로 바뀐 스탯만 담는다. */
struct UseItemResult
{
    Protocol::Slot updatedSlot;
    RepeatedPtrField<Protocol::Stat> updatedStats;
};

/**
 * 장비 착용과 해제의 결과. gearType은 바뀐 장비 부위이고, templateId는 처리 뒤 그 부위의 아이템이다(해제면 0).
 * updatedSlots는 바뀐 장비 칸과 인벤토리 칸, updatedStats는 값이 바뀐 스탯이다.
 */
struct GearChangeResult
{
    int32 gearType = 0;
    int32 templateId = 0;
    RepeatedPtrField<Protocol::Slot> updatedSlots;
    RepeatedPtrField<Protocol::Stat> updatedStats;
};

/** 리스폰 결과. updatedStats는 사망 패널티와 회복으로 바뀐 스탯이다. */
struct RespawnResult
{
    RepeatedPtrField<Protocol::Stat> updatedStats;
};

/**
 * 플레이어의 스폰 매개변수.
 * session이 비어 있으면 세션에 연결하지 않는다. 운영 코드는 언제나 세션을 넘기고, 빈 세션은 테스트만 쓴다.
 * progress가 있으면 그 진행으로 플레이어를 채우고 검증까지 한다. 검증에 실패하면 팩토리가 nullptr를 돌려준다.
 */
struct PlayerSpawnParams : public Creature::SpawnParams
{
    GameSessionRef session;
    /** 입장 요청을 받은 핸들러가 읽은 계정 번호. 세션에서 다시 읽지 않는다. */
    int64 userId = 0;
    /** 불러온 진행. Init 안에서만 읽으므로 Init이 끝날 때까지만 살아 있으면 된다. */
    const PlayerProgress* progress = nullptr;
};

/**
 * 접속한 캐릭터 하나를 나타내는 Creature.
 * Tick을 돌리지 않는다.
 */
class Player : public Creature
{
public:
    using SpawnParams = PlayerSpawnParams;

	Player();
	virtual ~Player();

    /**
     * 플레이어 정보를 모두 채운 시점에 호출하는 함수. 언리얼의 FinishSpawning에 대응한다.
     * 불러온 스탯을 계산 결과와 대조하고 최종 스탯을 쓴다. 대조 결과가 맞지 않는다면 false.
     */
    bool OnLoaded();

protected:
    friend class EntityFactory;
    
    /**
     * 세션을 가리키고 인벤토리와 장비를 만든다. 세션의 _player는 바꾸지 않는다. 등록은 검증을 마친 호출자가 한다.
     * progress가 있으면 ApplyProgress로 채운 뒤 OnLoaded로 검증한다.
     */
    bool Init(const SpawnParams& params);
    
    //~ Begin Entity Interface
    virtual void Start() override;
    virtual void Tick(float deltaTime) override;
    //~ End Entity Interface

public:
    //~ 요청 처리

    /** 골드가 모자라거나 가방에 넣지 못하면 nullopt. 결과의 gold는 남은 골드다. */
    optional<BuyItemResult> ProcessBuyItem(int32 templateId, int32 count = 1);
    /** 팔 수 없는 아이템이거나 요청이 서버 슬롯과 어긋나면 nullopt. 결과의 gold는 남은 골드다. */
    optional<SellItemResult> ProcessSellItem(const Protocol::Slot& requestSlot, int32 count = 1);
    /**
     * nowMs는 재사용 대기 판정에 쓰는 현재 시각(ms)이다. 룸은 GetTickCount64()를 넘긴다.
     * 사망했거나, 소모품 칸이 아니거나, 재사용 대기 중이면 nullopt.
     */
    optional<UseItemResult> ProcessUseItem(const Protocol::Slot& requestSlot, uint64 nowMs);
    /** 결과의 gearType은 요청한 인벤토리 칸이 아니라 장착된 장비 부위다. 착용 조건에 맞지 않으면 nullopt. */
    optional<GearChangeResult> ProcessEquipGear(const Protocol::Slot& requestSlot);
    /** 가방에 자리가 없으면 장비 칸을 비우기 전에 거절한다(nullopt). 결과의 templateId는 0이다. */
    optional<GearChangeResult> ProcessUnequipGear(const Protocol::Slot& requestSlot);
    /**
     * 소속 룸이 없으면 nullopt. 위치를 respawnPos로 옮기고 사망 표시를 지운다.
     * 마을 리스폰만 경험치 감소(최대 경험치의 10%)와 HP 절반 회복을 적용한다.
     */
    optional<RespawnResult> ProcessRespawn(Protocol::RespawnType type, const Protocol::PosInfo& respawnPos);

    //~ 이벤트
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    /** 들어갈 맵과 룸 id를 기록한다. 룸 입장은 목적지 룸 큐의 OnEnterRoom에서 끝난다. */
    void OnEnterMap(int32 mapId, int32 roomId);
    /** 입장하는 룸의 큐 위에서 부른다. enterPos가 없으면 룸 중심에 둔다. */
    void OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos);
    /** 경험치가 남는 만큼 여러 레벨을 한 번에 올린다. 최대 레벨에서는 경험치를 버린다. */
    void OnGetReward(OUT Protocol::S_REWARD_RESULT& rewardResultPkt);
    /** 최대 레벨이면 아무것도 하지 않는다. */
    void OnLevelUp();

    //~ Player 정보 관련
    /** 이동 중인 룸이 없으면 -1이다. */
    int32 GetEnteringRoomId() { return _enteringRoomId; }
    /** 최대 레벨은 직업 레벨 표의 마지막 레벨이다. 레벨 표가 없는 직업도 true다. */
    bool IsMaxLevel() const;
    /** 마을(Gamedata::GetTownRoomId)의 룸과 리스폰 위치를 찾는다. 마을 룸이나 그 룸의 리스폰 지점이 없으면 false. */
    bool FindTownRespawnPoint(OUT RoomRef& respawnRoom, OUT Protocol::PosInfo& respawnPos);

    //~ 접속 종료
    /** 룸 큐 위에서만 부른다. */
    PlayerSaveData MakeSaveData() const;
    /** 사망한 채 끊긴 플레이어를 저장 직전에 마을 리스폰 상태로 바꾼다. 마을 리스폰 지점이 없으면 false. */
    bool ApplyTownRespawnForSave();

private:
    //~ 불러오기
    /**
     * 불러온 진행을 플레이어에 쓴다. 인벤토리와 장비는 슬롯 번호대로 넣는다. 스탯 검증은 OnLoaded가 한다.
     * 넣을 수 없는 슬롯이 하나라도 있으면 사유를 로그에 남기고 false. 그 행은 DB에 그대로 남는다.
     */
    bool ApplyProgress(const PlayerProgress& progress);

    //~ 스탯
    /**
     * 레벨 표의 기본 스탯에 착용 장비의 증감량을 더한다. 최종 스탯은 이 함수로만 계산한다.
     * 레벨 표에 없는 직업이나 레벨이면 nullopt.
     */
    optional<CombatStats> CalculateFinalStat();
    /**
     * 최종 스탯을 다시 계산해 쓰고, 현재 HP와 MP를 새 최대치로 자른다. 장착, 해제, 입장이 모두 이 함수를 거친다.
     * 값이 바뀐 스탯을 updatedStats에 싣는다. nullptr이면 싣지 않는다. 계산할 수 없으면 아무것도 바꾸지 않는다.
     */
    void RefreshFinalStat(OUT RepeatedPtrField<Protocol::Stat>* updatedStats);
    /** 불러온 현재 HP와 MP가 최대치를 넘지 않고 공격력이 계산 결과와 같은지 본다. 어긋나면 사유를 남기고 false. */
    bool ValidateLoadedStat();

    //~ 내부 계산
    void CacheNextLevelUpData();
    /** 다른 플레이어에게 보일 장비 외형 요약을 _playerInfo에 다시 쓴다. */
    void RefreshEquippedGearSummary();

public:
    //~ 세션
    /** 세션이 끊겨 사라졌거나 세션 없이 만든 플레이어(테스트)면 nullptr. */
    GameSessionRef GetSession() const { return _session.lock(); }
    /** 세션은 끊긴 뒤 사라질 수 있어서 저장에 쓸 계정 번호를 따로 든다. Init 뒤로는 바뀌지 않는다. */
    int64 GetUserId() const { return _userId; }
    /** 접속 종료를 표시한다. 세션 스레드(GameSession::OnDisconnected)가 부른다. */
    void MarkDisconnected() { _disconnected.store(true); }
    /**
     * 룸 큐(Room::EnterPlayer)가 읽는다. 룸 이동 중에 끊겼으면 다음 룸이 이 표시를 보고 퇴장과 저장을 이어 받는다.
     * 세션 스레드가 쓰고 룸 큐가 읽으므로 atomic이다.
     */
    bool IsDisconnected() const { return _disconnected.load(); }

    //~ 상태 읽기
    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_playerInfo; }
    const Protocol::Possession& GetPossession() const { return *_possession; }

    //~ 컴포넌트. 읽기만 연다. 소지품을 바꾸는 길은 Process* 요청 처리와 불러오기뿐이다.
    const InventoryComponent& GetInventory() const { return *_inventory; }
    const EquipmentComponent& GetEquipment() const { return *_equipment; }

private:
    /** 테스트가 준비 단계에서 레벨, 직업, 골드, 소지품을 직접 채운다(GameServerTests/PlayerTestAccess.h). */
    friend struct PlayerTestAccess;

	weak_ptr<GameSession> _session;
    int64 _userId = 0;
    atomic<bool> _disconnected = false;

    /** _entityInfo 안의 player_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::PlayerInfo* _playerInfo;
    /** 플레이어가 소유한다. */
    unique_ptr<Protocol::Possession> _possession;

    /** Init에서 만든다. */
    InventoryComponentRef _inventory;
    /** Init에서 만든다. */
    EquipmentComponentRef _equipment;

    int32 _enteringRoomId = -1;         // 이동하고자 하는 Room id

    NextLevelUpData _nextLevelUpData;
};

