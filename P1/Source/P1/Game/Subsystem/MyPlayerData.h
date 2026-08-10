// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Protocol.pb.h"
#include "MyPlayerData.generated.h"

class AP1MyPlayer;
class UInventory;
class UEquippedGear;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMyPlayerSpawned, AP1MyPlayer*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatChanged, int64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int64);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInvenSlotChanged, const Protocol::Slot&, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChanged, const Protocol::Slot&);

/**
 * 내 플레이어의 정보를 저장하는 클래스
 */
UCLASS()
class P1_API UMyPlayerData : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    void InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt);
    void BindMyPlayerDelegate(AP1MyPlayer* MyPlayer);

public:
    /** Setter 함수 */
    void SetObjectInfo(const Protocol::ObjectInfo& InObjectInfo);
    void SetStatValue(Protocol::StatType statType, const int64& value);
    void SetRoomId(int32 RoomId) { _PlayerInfo->set_room_id(RoomId); }
    void SetMapId(int32 MapId) { _PlayerInfo->set_map_id(MapId); }

    /** Getter 함수 */
    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_PlayerInfo; }
    const Protocol::StatInfo& GetStatInfo() const { return *_StatInfo; }
    int32 GetPlayerLevel() const { return _PlayerInfo->level(); };
    int64 GetStatValue(Protocol::StatType statType);

    Protocol::Possession* GetPossession() { return _Possession; }
    int32 GetGold() const { return _Possession->gold(); };
    UInventory* GetInventory() const { return Inventory; }
    UEquippedGear* GetEquippedGear() const { return EquippedGear; }

    uint64 GetPlayerId() const { return _PlayerId; }
    int32 GetRoomId() const { return _PlayerInfo->map_id(); }

    /** Replication 함수 */
    void Rep_GoldChanged(int64 Gold) const;
    void Rep_LevelChanged(int32 Level) const;
    void Rep_StatChanged(const Protocol::StatInfo& InStatInfo) const;
    void Rep_HpChanged(int64 UpdatedHp);

public:
    /** 델리게이트 */
    FOnMyPlayerSpawned OnMyPlayerSpawned;
    TMap<Protocol::StatType, FOnStatChanged> OnStatChangedMappings;
    FOnLevelChanged OnLevelChanged;
    FOnGoldChanged OnGoldChanged;
    FOnInvenSlotChanged OnInvenSlotChanged;
    FOnEquipmentSlotChanged OnEquipmentSlotChanged;

protected:
    UPROPERTY()
    UInventory* Inventory;

    UPROPERTY()
    UEquippedGear* EquippedGear;

    Protocol::ObjectInfo* _ObjectInfo;
    Protocol::PlayerInfo* _PlayerInfo;
    Protocol::StatInfo* _StatInfo;
    Protocol::Possession* _Possession;

    // Cached Data
    uint64 _PlayerId = 0;                           
    FText _PlayerName = FText::FromString("NULL");
};
