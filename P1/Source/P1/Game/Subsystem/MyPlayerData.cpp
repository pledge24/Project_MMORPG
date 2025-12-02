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
    _PlayerInfo = new Protocol::PlayerInfo();

    OnMyPlayerSpawned.AddUObject(this, &UMyPlayerData::BindMyPlayerSpawned);
}

void UMyPlayerData::Deinitialize()
{
    Inventory = nullptr;
    EquippedGear = nullptr;

    delete _PlayerInfo;
    _PlayerInfo = nullptr;

    Super::Deinitialize();
}

void UMyPlayerData::InitMyPlayerData(const Protocol::ObjectInfo& InObjectInfo)
{
    const Protocol::PlayerInfo& PlayerInfo = InObjectInfo.player_info();

    _PlayerId = InObjectInfo.object_id();
    _PlayerName = FText::FromString(UTF8_TO_TCHAR(PlayerInfo.name().c_str()));
    _PlayerInfo->CopyFrom(PlayerInfo);

    Inventory->Init(_PlayerInfo->mutable_inventory());
    EquippedGear->Init(_PlayerInfo);

}

void UMyPlayerData::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    if (IsValid(MyPlayer) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("MyPlayer Is InValid"));
        return;
    }

    /** Creature Class Delegate */
    MyPlayer->OnStatInfoChanged.AddUObject(this, &UMyPlayerData::Rep_StatChanged);
    MyPlayer->OnHpChanged.AddUObject(this, &UMyPlayerData::Rep_HpChanged);

    /** Player Class Delegate */
    MyPlayer->OnLevelChanged.AddUObject(this, &UMyPlayerData::Rep_LevelChanged);

    /** MyPlayer Class Delegate */
    MyPlayer->OnExpChanged.AddUObject(this, &UMyPlayerData::Rep_ExpChanged);
    MyPlayer->OnGoldChanged.AddUObject(this, &UMyPlayerData::Rep_GoldChanged);
    MyPlayer->OnInvenSlotChanged.AddUObject(Inventory, &UInventory::Rep_SlotChanged);
    MyPlayer->OnGearSlotChanged.AddUObject(EquippedGear, &UEquippedGear::Rep_SlotChanged);
}

void UMyPlayerData::Rep_GoldChanged(const int64 Gold) const
{
    _PlayerInfo->set_gold(Gold);
}

void UMyPlayerData::Rep_LevelChanged(int32 Level) const
{
    _PlayerInfo->set_level(Level);
}

void UMyPlayerData::Rep_ExpChanged(TOptional<int32> CurExp, TOptional<int32> MaxExp) const
{
    if (CurExp.IsSet())
        _PlayerInfo->set_cur_exp(CurExp.GetValue());
    if (MaxExp.IsSet())
        _PlayerInfo->set_max_exp(MaxExp.GetValue());
}

void UMyPlayerData::Rep_StatChanged(const Protocol::StatInfo& InStatInfo) const
{
    _PlayerInfo->mutable_stat_info()->CopyFrom(InStatInfo);
}

void UMyPlayerData::Rep_HpChanged(int32 UpdatedHp)
{
    _PlayerInfo->mutable_stat_info()->set_hp(UpdatedHp);
}
