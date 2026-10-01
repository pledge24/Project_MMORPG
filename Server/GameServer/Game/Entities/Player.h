#pragma once
#include "Creature.h"
#include "PlayerSaveData.h"

class GameSession;
class Room;
class Inventory;
class EquippedGear;
struct RoomEnterData;

struct NextLevelUpData
{
    int32 level = 0;
    int64 maxHpIncrement = 0;
    int64 maxMpIncrement = 0;
    int64 paIncrement = 0;
    int64 maIncrement = 0;
    int64 expRequirement = 0;
};

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

public:
    virtual bool Init() override;
    virtual bool Start() override;

protected:
    virtual void Tick(float deltaTime) override {};

public:
    /** 핸들 함수 */
    bool ProcessBuyItem(OUT RepeatedPtrField<Protocol::Slot>* updatedSlots, OUT int64& totalGold, int32 templateId, int32 count = 1);
    bool ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count = 1);
    // nowMs는 재사용 대기 판정에 쓰는 현재 시각이다. 룸은 GetTickCount64()를 넘긴다.
    bool ProcessUseItem(const Protocol::Slot& requestSlot, uint64 nowMs, OUT Protocol::S_USE_ITEM& pkt);
    bool ProcessEquipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_EQUIP_GEAR& pkt);
    bool ProcessUnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_UNEQUIP_GEAR& pkt);
    bool ProcessRespawn(Protocol::RespawnType type, shared_ptr<Protocol::PosInfo> respawnPos, OUT Protocol::S_RESPAWN& pkt);

    /** 이벤트 함수 */
    virtual void OnHit(EntityRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(EntityRef attacker) override;

    void OnEnterMap(int32 mapId, int32 roomId);
    void OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos);
    void OnGetReward(OUT Protocol::S_REWARD_RESULT& rewardResultPkt);
    void OnLevelUp();

    /** Getter 함수*/
    int32 GetEnteringRoomId() { return _enteringRoomId; }
    bool IsMaxLevel() const;
    // 리스폰할 룸과 위치를 찾는다. 지원하지 않는 유형이거나 룸이나 리스폰 지점이 없으면 false.
    bool GetRespawnData(Protocol::RespawnType respawnType, OUT RoomRef& respawnRoom, OUT Protocol::PosInfo& respawnPos);

    /** 접속 종료 */
    // 룸 큐 위에서만 부른다.
    PlayerSaveData MakeSaveData() const;
    bool ApplyTownRespawnForSave();

private:
    /** 기타 함수 */
    bool CalculateFinalStat();
    void CacheNextLevelUpData();
    void RefreshEquippedGearSummary();

public:
	weak_ptr<GameSession> _session;
    int64 _userId = 0;                   // 세션은 끊긴 뒤 사라질 수 있어서 저장에 쓸 값을 따로 들고 있다

    // 접속 종료 표시. 세션 스레드가 쓰고 룸 큐가 읽는다.
    // 룸 이동 중에 끊기면 다음 룸의 EnterPlayer가 이 표시를 보고 퇴장과 저장을 이어 받는다.
    atomic<bool> _disconnected = false;

    Protocol::PlayerInfo* _playerInfo;
    Protocol::Possession* _possession;

    InventoryRef _inventory;             
    EquippedGearRef _equippedGear;       

private:
    int32 _enteringRoomId = -1;         // 이동하고자 하는 Room id

    const int32 MAX_LEVEL = 50;
    NextLevelUpData _nextLevelUpData;

    // 소모품 템플릿 id → 마지막으로 쓴 시각(ms). 재사용 대기 판정에 쓴다. 저장하지 않으므로 재접속하면 사라진다.
    map<int32, uint64> _lastUseTimeMs;
    const int32 RESPAWN_TOWN_ID = 10;   // 고정으로 사용
};

