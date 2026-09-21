#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Protocol.pb.h"
#include "P1MyPlayerData.generated.h"

class AP1MyPlayer;
class UP1Inventory;
class UP1EquippedGear;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMyPlayerSpawned, AP1MyPlayer*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatChanged, int64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int64);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInvenSlotChanged, const Protocol::Slot&, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChanged, const Protocol::Slot&);

/** 내 플레이어의 정보를 담는 서브시스템이다. */
UCLASS()
class P1_API UP1MyPlayerData : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Begin USubsystem Interface
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Lifecycle
public:
    void InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt);
    void BindMyPlayerDelegate(AP1MyPlayer* MyPlayer);

    FOnMyPlayerSpawned OnMyPlayerSpawned;

    //~ Player Info
public:
    void SetEntityInfo(const Protocol::EntityInfo& InEntityInfo);
    void SetRoomId(int32 RoomId) { _PlayerInfo->set_room_id(RoomId); }
    void SetMapId(int32 MapId) { _PlayerInfo->set_map_id(MapId); }

    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_PlayerInfo; }
    uint64 GetPlayerId() const { return _PlayerId; }
    int32 GetPlayerLevel() const { return _PlayerInfo->level(); };

    // 룸 ID와 맵 ID는 서로 다른 번호 공간이다(룸 10/20/30/40, 맵 1111).
    int32 GetRoomId() const { return _PlayerInfo->room_id(); }
    int32 GetMapId() const { return _PlayerInfo->map_id(); }

    void Rep_LevelChanged(int32 Level) const;

    FOnLevelChanged OnLevelChanged;

protected:
    Protocol::EntityInfo* _EntityInfo;
    Protocol::PlayerInfo* _PlayerInfo;

    uint64 _PlayerId = 0;
    FText _PlayerName = FText::FromString("NULL");

    //~ Stats
public:
    void SetStatValue(Protocol::StatType statType, const int64& value);

    const Protocol::StatInfo& GetStatInfo() const { return *_StatInfo; }
    int64 GetStatValue(Protocol::StatType statType);

    void Rep_StatChanged(const Protocol::StatInfo& InStatInfo) const;
    void Rep_HpChanged(int64 UpdatedHp);

    TMap<Protocol::StatType, FOnStatChanged> OnStatChangedMappings;

protected:
    Protocol::StatInfo* _StatInfo;

    //~ Possession
public:
    Protocol::Possession* GetPossession() { return _Possession; }
    int32 GetGold() const { return _Possession->gold(); };

    void Rep_GoldChanged(int64 Gold) const;

    FOnGoldChanged OnGoldChanged;

protected:
    Protocol::Possession* _Possession;

    //~ Inventory
public:
    UP1Inventory* GetInventory() const { return Inventory; }

    FOnInvenSlotChanged OnInvenSlotChanged;

protected:
    UPROPERTY()
    TObjectPtr<UP1Inventory> Inventory;

    //~ Equipment
public:
    UP1EquippedGear* GetEquippedGear() const { return EquippedGear; }

    FOnEquipmentSlotChanged OnEquipmentSlotChanged;

protected:
    UPROPERTY()
    TObjectPtr<UP1EquippedGear> EquippedGear;
};
