#include "Core/P1MyPlayerData.h"
#include "Game/Inventory/P1Inventory.h"
#include "Game/Equipment/P1EquippedGear.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Utils/LogCategory.h"

void UP1MyPlayerData::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create a Inventory Object
    Inventory = NewObject<UP1Inventory>(this, UP1Inventory::StaticClass());
    if (Inventory == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("Inventory Is Not Exist"));

    // Create a EquippedGear Object
    EquippedGear = NewObject<UP1EquippedGear>(this, UP1EquippedGear::StaticClass());
    if (EquippedGear == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("EquippedGear Is Not Exist"));

    // Proto
    _EntityInfo = new Protocol::EntityInfo();
    _PlayerInfo = _EntityInfo->mutable_player_info();
    _StatInfo = new Protocol::StatInfo();
    _Possession = new Protocol::Possession();

    // Add Stat Delegate(Common)
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_HP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAGICAL_ATTACK);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_MAX_EXP);
    OnStatChangedMappings.Add(Protocol::STAT_TYPE_EXP);

    // Bind Delegate
    OnMyPlayerSpawned.AddUObject(this, &UP1MyPlayerData::BindMyPlayerDelegate);

    // 소지품 델리게이트는 이 서브시스템과 함께 사는 객체끼리 잇는다. 플레이어 액터와 무관하므로 한 번만 붙인다.
    // 내 플레이어가 스폰될 때마다 붙이면 맵을 옮길 때마다 핸들러가 하나씩 늘어난다.
    OnGoldChanged.AddUObject(this, &UP1MyPlayerData::Rep_GoldChanged);
    OnInvenSlotChanged.AddUObject(Inventory, &UP1Inventory::Rep_SlotChanged);
    OnEquipmentSlotChanged.AddUObject(EquippedGear, &UP1EquippedGear::Rep_SlotChanged);
}

void UP1MyPlayerData::Deinitialize()
{
    Inventory = nullptr;
    EquippedGear = nullptr;

    delete _EntityInfo;
    delete _StatInfo;
    delete _Possession;

    _EntityInfo = nullptr;
    _PlayerInfo = nullptr;
    _StatInfo = nullptr;
    _Possession = nullptr;

    Super::Deinitialize();
}

void UP1MyPlayerData::InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    // 저장
    {
        SetEntityInfo(EnterGamePkt.player());
        _StatInfo->CopyFrom(EnterGamePkt.stat_info());
        _Possession->CopyFrom(EnterGamePkt.possession());
        
        // Cache
        _PlayerId = _EntityInfo->entity_id();
        _PlayerName = FText::FromString(UTF8_TO_TCHAR(_EntityInfo->player_info().name().c_str()));
    }

    // Initialize Possession Wrapper
    {
        Inventory->Init(_Possession->mutable_inventory());
        EquippedGear->Init(_Possession->mutable_equipped_gear());
    }

}

void UP1MyPlayerData::BindMyPlayerDelegate(AP1MyPlayer* MyPlayer)
{
    if (IsValid(MyPlayer) == false)
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("MyPlayer Is InValid"));
        return;
    }

    // 내 플레이어 액터는 맵마다 새로 스폰되므로 액터의 델리게이트는 스폰 때마다 붙인다.
    MyPlayer->OnLevelUp.AddUObject(this, &UP1MyPlayerData::Rep_LevelChanged);
}

void UP1MyPlayerData::RemoveListener(const UObject* Listener)
{
    OnMyPlayerSpawned.RemoveAll(Listener);
    OnLevelChanged.RemoveAll(Listener);
    OnGoldChanged.RemoveAll(Listener);
    OnInvenSlotChanged.RemoveAll(Listener);
    OnEquipmentSlotChanged.RemoveAll(Listener);

    for (auto& Pair : OnStatChangedMappings)
        Pair.Value.RemoveAll(Listener);
}

void UP1MyPlayerData::SetEntityInfo(const Protocol::EntityInfo& InEntityInfo)
{
    // 프로토버프는 메세지를 CopyFrom할때마다 필트의 포인터가 달라질 수 있다.
    _EntityInfo->CopyFrom(InEntityInfo);
    _PlayerInfo = _EntityInfo->mutable_player_info();
}

void UP1MyPlayerData::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _StatInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

void UP1MyPlayerData::ApplyStat(Protocol::StatType statType, int64 value)
{
    SetStatValue(statType, value);

    if (FOnStatChanged* OnThisStatChanged = OnStatChangedMappings.Find(statType))
        OnThisStatChanged->Broadcast(value);
}

int64 UP1MyPlayerData::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = _StatInfo->mutable_info();
    return statMappings->at((int32)statType);
}

void UP1MyPlayerData::Rep_GoldChanged(const int64 Gold) const
{
    _Possession->set_gold(Gold);
}

void UP1MyPlayerData::Rep_LevelChanged(int32 Level) const
{
    _PlayerInfo->set_level(Level);
}
