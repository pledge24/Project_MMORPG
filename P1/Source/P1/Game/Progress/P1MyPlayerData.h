#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Protocol.pb.h"
#include "Game/Inventory/P1ItemCooldown.h"
#include "Game/Items/P1ContainerType.h"
#include "P1MyPlayerData.generated.h"

class AP1MyPlayer;
class UP1ItemActions;
class UP1ItemContainer;
class UP1QuickSlots;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMyPlayerSpawned, AP1MyPlayer*);
DECLARE_MULTICAST_DELEGATE(FOnMapEntered);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatChanged, int64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int64);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemCooldownStarted, int32);

/**
 * 내 플레이어의 상태.
 *
 * 서버가 알려 준 내 플레이어의 정보, 스탯, 소지품, 재사용 대기의 사본을 들고, 바뀌면 구독자에게 알린다.
 * 레벨과 무관하게 살아 있어 화면이 언제든 현재 값을 읽을 수 있다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1MyPlayerData : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UP1MyPlayerData();

    //~ Begin USubsystem Interface
public:
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Lifecycle
public:
    void InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt);

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

    /** 룸 ID와 맵 ID는 서로 다른 번호 공간이다. */
    int32 GetRoomId() const { return _PlayerInfo->room_id(); }
    int32 GetMapId() const { return _PlayerInfo->map_id(); }

    /** 레벨을 사본에 쓰고 구독자에게 알린다. 내 플레이어의 레벨은 이 함수로만 바꾼다. */
    void ApplyLevel(int32 Level);

    FOnLevelChanged OnLevelChanged;

protected:
    TUniquePtr<Protocol::EntityInfo> _EntityInfo;
    /** _EntityInfo 안의 PlayerInfo를 가리킨다. */
    Protocol::PlayerInfo* _PlayerInfo;

    uint64 _PlayerId = 0;
    FText _PlayerName = FText::FromString("NULL");

    //~ Stats
public:
    /** 사본에만 쓰고 구독자에게 알리지 않는다. 내 플레이어의 스탯을 바꿀 때는 ApplyStat을 쓴다. */
    void SetStatValue(Protocol::StatType statType, const int64& value);

    /** 서버가 알려 준 스탯을 사본에 쓰고 구독자에게 알린다. 내 플레이어의 스탯은 이 함수로만 바꾼다. */
    void ApplyStat(Protocol::StatType statType, int64 value);

    /** 패킷에 실린 스탯 목록을 ApplyStat으로 차례로 반영한다. */
    void ApplyStats(const google::protobuf::RepeatedPtrField<Protocol::Stat>& Stats);

    const Protocol::StatInfo& GetStatInfo() const { return *_StatInfo; }
    int64 GetStatValue(Protocol::StatType statType);

    TMap<Protocol::StatType, FOnStatChanged> OnStatChangedMappings;

protected:
    TUniquePtr<Protocol::StatInfo> _StatInfo;

    //~ Possession
public:
    int64 GetGold() const { return _Gold; }

    /** 골드를 사본에 쓰고 구독자에게 알린다. 내 플레이어의 골드는 이 함수로만 바꾼다. */
    void ApplyGold(int64 Gold);

    FOnGoldChanged OnGoldChanged;

protected:
    int64 _Gold = 0;

    //~ Containers
public:
    /** 인벤토리의 구역과 착용 장비의 컨테이너. 컨테이너가 아닌 종류(None)면 nullptr. */
    UP1ItemContainer* GetContainer(EP1ContainerType ContainerType) const;

    /** 모든 컨테이너의 칸을 지운다. 접속이 끊기면 부른다. */
    void ClearContainers();

protected:
    /** 서브시스템과 함께 살고, 입장과 응답으로 칸만 바뀐다. */
    UPROPERTY()
    TMap<EP1ContainerType, TObjectPtr<UP1ItemContainer>> Containers;

    //~ Quick Slots
public:
    /** 서브시스템과 함께 살고, 입장과 등록 응답으로 대상만 바뀐다. */
    UP1QuickSlots* GetQuickSlots() const { return QuickSlots; }

    /** 퀵슬롯을 모두 지운다. 접속이 끊기면 부른다. */
    void ClearQuickSlots();

    /** 등록 응답의 바뀐 퀵슬롯을 반영한다. */
    void HandleSetQuickSlot(const Protocol::S_SET_QUICK_SLOT& SetQuickSlotPkt);

protected:
    UPROPERTY()
    TObjectPtr<UP1QuickSlots> QuickSlots;

    //~ Item Cooldown
public:
    /** 템플릿이 재사용 대기 중이면 그 대기를, 아니면 nullptr. 시각은 FP1ItemCooldown::GetClockSeconds()로 잰다. */
    const FP1ItemCooldown* FindActiveItemCooldown(int32 TemplateId) const;

    bool IsItemCoolingDown(int32 TemplateId) const { return FindActiveItemCooldown(TemplateId) != nullptr; }

    /** 재사용 대기가 시작된 템플릿 id를 알린다. 대기는 템플릿 단위라 같은 아이템이 든 칸이 모두 대기한다. */
    FOnItemCooldownStarted OnItemCooldownStarted;

private:
    /** 아이템 정의의 Cooldown(초)으로 대기를 시작한다. 대기가 없는 아이템이면 아무것도 하지 않는다. */
    void StartItemCooldown(int32 TemplateId);

    /** 템플릿 id별 재사용 대기. 서버처럼 재접속하면 비운다. */
    TMap<int32, FP1ItemCooldown> ItemCooldowns;

    //~ Enter Packet Handlers
public:
    /** 성공하면 맵과 룸 id를 저장하고 OnMapEntered를 알린다. */
    void HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt);

    /** 성공하면 룸 id를 저장한다. 엔티티 정리는 엔티티 관리자가 맡는다. */
    void HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt);

    /** 맵 입장에 성공하면 알린다. */
    FOnMapEntered OnMapEntered;

    //~ Item Requests
public:
    /** 아이템 요청 창구. 서브시스템과 함께 산다. */
    UP1ItemActions* GetItemActions() const { return ItemActions; }

protected:
    UPROPERTY()
    TObjectPtr<UP1ItemActions> ItemActions;

    //~ Item Packet Handlers
public:
    void HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt);
    void HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt);

    /** 내 플레이어의 사용 응답이 아니면 무시한다. */
    void HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt);

    /** 내 플레이어의 응답이 아니면 무시한다. 외형은 엔티티 관리자가 바꾼다. */
    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);

    /** 내 플레이어의 응답이 아니면 무시한다. 외형은 엔티티 관리자가 바꾼다. */
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

private:
    bool IsMyPlayer(uint64 EntityId) const { return EntityId == _PlayerId; }

    /** 응답이 실어 온 칸 변경을 그 칸의 컨테이너에 나눠 준다. 컨테이너가 아닌 칸의 변경은 버린다. */
    void ApplySlotChanges(const google::protobuf::RepeatedPtrField<Protocol::SlotChange>& Changes);

    //~ Reward Packet Handlers
public:
    /** 보상으로 바뀐 레벨, 스탯, 경험치, 골드를 반영한다. 서버가 계산한 최종 값을 그대로 쓴다. */
    void HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt);

    //~ Respawn
public:
    /** 서버가 마을 리스폰 요청을 거절하면 알린다. 정상 흐름에서는 일어나지 않는다. */
    DECLARE_MULTICAST_DELEGATE(FOnTownRespawnRejected);
    FOnTownRespawnRejected OnTownRespawnRejected;
};
