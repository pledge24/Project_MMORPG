#pragma once
#include "Creature.h"

class GameSession;
class Room;
class Inventory;
class EquippedGear;
struct RoomEnterData;

struct NextLevelUpData
{
    uint32 level = 0;
    uint64 maxHpIncrement = 0;
    uint64 maxMpIncrement = 0;
    uint64 paIncrement = 0;
    uint64 maIncrement = 0;
    uint64 expRequirement = 0;
};

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

protected:
    virtual void PostConstructionSetup() override;
    virtual void Tick(float deltaTime) override;

public:
    void Init();
    bool PostInit();
    bool CalculateFinalStat();

    /* 핸들 함수 */
    bool HandleBuyItem(OUT Protocol::Slot* updatedSlot, OUT int64& totalGold, int32 templateId, int32 count = 1);
    bool HandleSellItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, OUT int64& totalGold, int32 count = 1);
    bool HandleUseItem(OUT Protocol::S_USE_ITEM& pkt, Protocol::Slot* targetSlot);

    bool HandleEquipGear(OUT Protocol::S_EQUIP_GEAR& pkt, Protocol::Slot* targetSlot);
    bool HandleUnequipGear(OUT Protocol::S_UNEQUIP_GEAR& pkt, Protocol::Slot* targetSlot);

    /** 이벤트 함수 */
    virtual void OnHit(ObjectRef attacker, Protocol::HitData& hitData) override;
    virtual void OnEnterRoom(RoomRef enterRoom, const optional<Protocol::PosInfo>& enterPos);

    void OnMonsterKill(MonsterRef killedMonster, uint64 expReward, uint64 goldReward);
    void OnLevelUp();
    void OnRespawn();

    /** Getter */
    uint32 GetRespawnRoomId(Protocol::RespawnType respawnType) { return respawnRoomMappings[respawnType]; }

private:
    /** 기타 함수 */
    void CacheNextLevelUpData();

public:
	weak_ptr<GameSession> session;

    Protocol::PlayerInfo* playerInfo;   // 플레이어의 모든 정보가 여기에 저장됨.
    Protocol::StatInfo* statInfo;

    InventoryRef inventory;             // 인벤토리 헬퍼
    EquippedGearRef equippedGear;       // 장착 아이템 헬퍼

private:
    const uint32 MAX_LEVEL = 50;
    NextLevelUpData _nextLevelUpData;

    map<Protocol::RespawnType, uint32> respawnRoomMappings;
    uint32 RESPAWN_TOWN_ID = 10;        // 고정으로 사용
};

