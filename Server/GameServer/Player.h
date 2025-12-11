#pragma once
#include "Creature.h"

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
    virtual bool Init(Protocol::PosInfo* spawnPos = nullptr) override;
    virtual bool Start() override;

protected:
    virtual void Tick(float deltaTime) override {};

public:
    bool CalculateFinalStat();

    /* 핸들 함수 */
    bool ProcessBuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count = 1);
    bool ProcessSellItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 count = 1);
    bool ProcessUseItem(const Protocol::Slot& requestSlot, OUT Protocol::S_USE_ITEM& pkt);
    bool ProcessEquipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_EQUIP_GEAR& pkt);
    bool ProcessUnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::S_UNEQUIP_GEAR& pkt);
    bool ProcessRespawn(Protocol::RespawnType type, shared_ptr<Protocol::PosInfo> respawnPos, OUT Protocol::S_RESPAWN& pkt);

    /** 이벤트 함수 */
    virtual void OnHit(ObjectRef attacker, Protocol::AttackInfo attackInfo) override;
    virtual void OnDie(ObjectRef attacker) override;

    void OnEnterMap(int32 mapId, int32 roomId);
    void OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos);
    void OnReward(OUT Protocol::S_REWARD_RESULT& rewardResultPkt);
    void OnLevelUp();
  
    /** Getter */
    int32 GetRespawnRoomId(Protocol::RespawnType respawnType) { return respawnRoomMappings[respawnType]; }
    int32 GetEnteringRoomId() { return enteringRoomId; }

private:
    /** 기타 함수 */
    void CacheNextLevelUpData();

public:
	weak_ptr<GameSession> session;

    Protocol::PlayerInfo* playerInfo;   // 플레이어의 모든 정보가 여기에 저장됨.
    Protocol::Possession* possession;

    InventoryRef inventory;             // 인벤토리 헬퍼
    EquippedGearRef equippedGear;       // 장착 아이템 헬퍼

private:
    int32 enteringRoomId = -1;          // 이동하고자 하는 Room id

    const int32 MAX_LEVEL = 50;
    NextLevelUpData _nextLevelUpData;

    map<Protocol::RespawnType, int32> respawnRoomMappings;
    int32 RESPAWN_TOWN_ID = 10;        // 고정으로 사용
};

