#pragma once
#include "Game/Entities/Creature.h"
#include "Game/Entities/PlayerSaveData.h"

class GameSession;
class Room;
class Inventory;
class EquippedGear;
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

/**
 * 접속한 캐릭터 하나를 나타내는 크리처.
 * 게임 입장 때 DB 스레드의 불러오기 잡에서 만들고(EntityUtils::CreatePlayer) DB 값을 채운 뒤 Start한다.
 * 룸에 들어간 뒤로는 소속 룸 큐 위에서만 다룬다. 예외는 atomic인 _room과 _disconnected다.
 * GameSession::_player와 룸의 _entities가 붙잡는다. 인벤토리와 장비는 플레이어를 weak_ptr로 가리킨다.
 * 틱을 돌지 않는다.
 */
class Player : public Creature
{
public:
	Player();
	virtual ~Player();

public:
    /** 인벤토리와 장비를 만든다. shared_from_this를 쓰므로 shared_ptr로 만든 뒤에만 부를 수 있다. */
    virtual bool Init() override;
    /**
     * DB 값을 모두 채운 뒤에 부른다. dirty flag를 지우고 최종 스탯을 다시 계산해 DB 값과 대조한다.
     * 레벨 표에 없는 직업이나 레벨이거나, 저장된 스탯이 계산 결과와 어긋나면 false.
     */
    virtual bool Start() override;

protected:
    virtual void Tick(float deltaTime) override {};

public:
    //~ 요청 처리
    /**
     * 아래 함수는 모두 소속 룸 큐 위에서 부른다.
     * 아이템 요청은 요청에 실린 아이템이 아니라 서버 슬롯에 든 아이템으로 판정한다.
     * 요청의 아이템이 서버 슬롯과 다르면 거절한다.
     */

    /** 골드가 모자라거나 가방에 넣지 못하면 false. 성공하면 totalGold에 남은 골드를 채운다. */
    bool ProcessBuyItem(OUT RepeatedPtrField<Protocol::Slot>* updatedSlots, OUT int64& totalGold, int32 templateId, int32 count = 1);
    /** 팔 수 없는 아이템이면 false. 성공하면 totalGold에 남은 골드를 채운다. */
    bool ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count = 1);
    /**
     * nowMs는 재사용 대기 판정에 쓰는 현재 시각(ms)이다. 룸은 GetTickCount64()를 넘긴다.
     * 거절해도 pkt에 entity_id를 싣는다. 슬롯은 성공했을 때만 싣는다.
     */
    bool ProcessUseItem(const Protocol::Slot& requestSlot, uint64 nowMs, OUT Protocol::S_USE_ITEM& pkt);
    /** pkt의 slot_id에는 요청한 인벤토리 칸이 아니라 장착된 장비 부위가 실린다. */
    bool ProcessEquipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_EQUIP_GEAR& pkt);
    /** 가방에 자리가 없으면 장비 칸을 비우기 전에 거절한다. 성공하면 pkt의 template_id는 0이다. */
    bool ProcessUnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_UNEQUIP_GEAR& pkt);
    /**
     * 소속 룸이 없으면 false. 위치를 respawnPos로 옮기고 사망 표시를 지운다.
     * 마을 리스폰만 경험치 감소(최대 경험치의 10%)와 HP 절반 회복을 적용한다.
     */
    bool ProcessRespawn(Protocol::RespawnType type, shared_ptr<Protocol::PosInfo> respawnPos, OUT Protocol::S_RESPAWN& pkt);

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

    //~ Getter
    /** 이동 중인 룸이 없으면 -1이다. */
    int32 GetEnteringRoomId() { return _enteringRoomId; }
    bool IsMaxLevel() const;
    /** 마을 리스폰의 룸과 위치를 찾는다. 마을 룸이나 그 룸의 리스폰 지점이 없으면 false. */
    bool FindTownRespawnPoint(OUT RoomRef& respawnRoom, OUT Protocol::PosInfo& respawnPos);

    //~ 접속 종료
    /** 룸 큐 위에서만 부른다. */
    PlayerSaveData MakeSaveData() const;
    /** 사망한 채 끊긴 플레이어를 저장 직전에 마을 리스폰 상태로 바꾼다. 마을 리스폰 지점이 없으면 false. */
    bool ApplyTownRespawnForSave();

private:
    //~ 내부 계산
    bool CalculateFinalStat();
    void CacheNextLevelUpData();
    /** 다른 플레이어에게 보일 장비 외형 요약을 _playerInfo에 다시 쓴다. */
    void RefreshEquippedGearSummary();

public:
	weak_ptr<GameSession> _session;
    int64 _userId = 0;                   // 세션은 끊긴 뒤 사라질 수 있어서 저장에 쓸 값을 따로 들고 있다

    /**
     * 접속 종료 표시. 세션 스레드가 쓰고 룸 큐가 읽는다.
     * 룸 이동 중에 끊기면 다음 룸의 EnterPlayer가 이 표시를 보고 퇴장과 저장을 이어 받는다.
     */
    atomic<bool> _disconnected = false;

    /** _entityInfo 안의 player_info를 가리킨다. 따로 지우지 않는다. */
    Protocol::PlayerInfo* _playerInfo;
    /** 플레이어가 소유한다. 소멸자에서 지운다. */
    Protocol::Possession* _possession;

    /** Init에서 만든다. */
    InventoryRef _inventory;             
    /** Init에서 만든다. */
    EquippedGearRef _equippedGear;       

private:
    int32 _enteringRoomId = -1;         // 이동하고자 하는 Room id

    const int32 MAX_LEVEL = 50;
    NextLevelUpData _nextLevelUpData;

    /** 소모품 템플릿 id → 마지막으로 쓴 시각(ms). 재사용 대기 판정에 쓴다. 저장하지 않으므로 재접속하면 사라진다. */
    map<int32, uint64> _lastUseTimeMs;
    const int32 RESPAWN_TOWN_ID = 10;   // 고정으로 사용
};

