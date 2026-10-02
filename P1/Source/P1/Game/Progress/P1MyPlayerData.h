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

    /**
     * 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다.
     * 서브시스템은 레벨보다 오래 살기 때문에, 레벨과 함께 사라지는 객체는 사라지기 전에 불러야 한다.
     */
    void RemoveListener(const UObject* Listener);

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
    TUniquePtr<Protocol::EntityInfo> _EntityInfo;
    Protocol::PlayerInfo* _PlayerInfo;

    uint64 _PlayerId = 0;
    FText _PlayerName = FText::FromString("NULL");

    //~ Stats
public:
    void SetStatValue(Protocol::StatType statType, const int64& value);

    /** 서버가 알려 준 스탯을 사본에 쓰고 구독자에게 알린다. 내 플레이어의 스탯은 이 함수로만 바꾼다. 게임 스레드 전용. */
    void ApplyStat(Protocol::StatType statType, int64 value);

    /** 패킷에 실린 스탯 목록을 ApplyStat으로 차례로 반영한다. 게임 스레드 전용. */
    void ApplyStats(const google::protobuf::RepeatedPtrField<Protocol::Stat>& Stats);

    const Protocol::StatInfo& GetStatInfo() const { return *_StatInfo; }
    int64 GetStatValue(Protocol::StatType statType);

    TMap<Protocol::StatType, FOnStatChanged> OnStatChangedMappings;

protected:
    TUniquePtr<Protocol::StatInfo> _StatInfo;

    //~ Possession
public:
    Protocol::Possession* GetPossession() { return _Possession.Get(); }
    int64 GetGold() const { return _Possession->gold(); }

    void Rep_GoldChanged(int64 Gold) const;

    FOnGoldChanged OnGoldChanged;

protected:
    /** Inventory와 EquippedGear가 이 객체의 하위 메시지 주소를 들고 있다. 새 객체로 바꾸지 않는다. */
    TUniquePtr<Protocol::Possession> _Possession;

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

    //~ Item Packet Handlers
public:
    /** 패킷 핸들러가 수신 펌프에서 부른다. 게임 스레드 전용. 아래 핸들러도 같다. */
    void HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt);
    void HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt);

    /** 내 플레이어의 사용 응답이 아니면 무시한다. */
    void HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt);

    /** 내 플레이어의 응답이 아니면 무시한다. 외형은 엔티티 관리자가 바꾼다. */
    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);

    /** 내 플레이어의 응답이 아니면 무시한다. 외형은 엔티티 관리자가 바꾼다. */
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

    /** 응답이 왔다는 알림이다. 성공 여부와 무관하게 알린다. */
    DECLARE_MULTICAST_DELEGATE(FOnRecvBuyItemPkt);
    FOnRecvBuyItemPkt OnRecvBuyItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvSellItemPkt);
    FOnRecvSellItemPkt OnRecvSellItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUseItemPkt);
    FOnRecvUseItemPkt OnRecvUseItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvEquipGearPkt);
    FOnRecvEquipGearPkt OnRecvEquipGearPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUnequipGearPkt);
    FOnRecvUnequipGearPkt OnRecvUnequipGearPkt;

private:
    bool IsMyPlayer(uint64 EntityId) const { return EntityId == _PlayerId; }

    /** 장착과 해제 응답이 실어 온 슬롯을 종류에 맞는 델리게이트로 알린다. */
    void ApplyGearSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& UpdatedSlots);
};
