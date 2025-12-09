// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerData.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "P1MyPlayer.h"

void UMyPlayerData::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create a Inventory Object
    Inventory = NewObject<UInventory>(this, UInventory::StaticClass());
    if (Inventory == nullptr)
        UE_LOG(LogTemp, Warning, TEXT("Inventory Is Not Exist"));

    // Create a EquippedGear Object
    EquippedGear = NewObject<UEquippedGear>(this, UEquippedGear::StaticClass());
    if (EquippedGear == nullptr)
        UE_LOG(LogTemp, Warning, TEXT("EquippedGear Is Not Exist"));

    // Proto
    _ObjectInfo = new Protocol::ObjectInfo();
    _PlayerInfo = _ObjectInfo->mutable_player_info();
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
    OnMyPlayerSpawned.AddUObject(this, &UMyPlayerData::BindMyPlayerDelegate);
}

void UMyPlayerData::Deinitialize()
{
    Inventory = nullptr;
    EquippedGear = nullptr;

    delete _ObjectInfo;
    delete _StatInfo;
    delete _Possession;

    _ObjectInfo = nullptr;
    _PlayerInfo = nullptr;
    _StatInfo = nullptr;
    _Possession = nullptr;

    Super::Deinitialize();
}

void UMyPlayerData::InitMyPlayerData(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    // 저장
    {
        SetObjectInfo(EnterGamePkt.player());
        _StatInfo->CopyFrom(EnterGamePkt.stat_info());
        _Possession->CopyFrom(EnterGamePkt.possession());
        
        // Cache
        _PlayerId = _ObjectInfo->object_id();
        _PlayerName = FText::FromString(UTF8_TO_TCHAR(_ObjectInfo->player_info().name().c_str()));
    }

    // Initialize Possession Wrapper
    {
        Inventory->Init(_Possession->mutable_inventory());
        EquippedGear->Init(_Possession->mutable_equipped_gear());
    }

}

void UMyPlayerData::BindMyPlayerDelegate(AP1MyPlayer* MyPlayer)
{
    if (IsValid(MyPlayer) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("MyPlayer Is InValid"));
        return;
    }

    /** Player Class Delegate */
    MyPlayer->OnLevelUp.AddUObject(this, &UMyPlayerData::Rep_LevelChanged);

    /** Possession Delegate */
    OnGoldChanged.AddUObject(this, &UMyPlayerData::Rep_GoldChanged);
    OnInvenSlotChanged.AddUObject(Inventory, &UInventory::Rep_SlotChanged);
    OnEquipmentSlotChanged.AddUObject(EquippedGear, &UEquippedGear::Rep_SlotChanged);
}

void UMyPlayerData::SetObjectInfo(const Protocol::ObjectInfo& InObjectInfo)
{
    // 프로토버프는 메세지를 CopyFrom할때마다 필트의 포인터가 달라질 수 있다.
    _ObjectInfo->CopyFrom(InObjectInfo);
    _PlayerInfo = _ObjectInfo->mutable_player_info();
}

void UMyPlayerData::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = _StatInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

int64 UMyPlayerData::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = _StatInfo->mutable_info();
    return statMappings->at((int32)statType);
}

void UMyPlayerData::Rep_GoldChanged(const int64 Gold) const
{
    _Possession->set_gold(Gold);
}

void UMyPlayerData::Rep_LevelChanged(int32 Level) const
{
    _PlayerInfo->set_level(Level);
}

void UMyPlayerData::Rep_StatChanged(const Protocol::StatInfo& InStatInfo) const
{
    _PlayerInfo->CopyFrom(InStatInfo);
}

void UMyPlayerData::Rep_HpChanged(int64 UpdatedHp)
{
    SetStatValue(Protocol::STAT_TYPE_HP, UpdatedHp);
}
