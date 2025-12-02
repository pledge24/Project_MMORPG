// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Protocol.pb.h"
#include "MyPlayerData.generated.h"

class AP1MyPlayer;
class UInventory;
class UEquippedGear;

/**
 * 월드에 종속되지 않는 내 플레이어의 정보를 저장하는 클래스
 */
UCLASS()
class P1_API UMyPlayerData : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    void InitMyPlayerData(const Protocol::ObjectInfo& InObjectInfo);
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    /** Getter함수 */
    UInventory* GetInventory() const { return Inventory; }
    UEquippedGear* GetEquippedGear() const { return EquippedGear; }

    const Protocol::PlayerInfo& GetPlayerInfo() const { return *_PlayerInfo; }
    const Protocol::StatInfo& GetStatInfo() const { return *_PlayerInfo->mutable_stat_info(); }

    int32 GetGold() const { return _PlayerInfo->gold(); };
    int32 GetPlayerLevel() const { return _PlayerInfo->level(); };
    uint64 GetPlayerId() const { return _PlayerId; }

    /** Replication 함수 */
    void Rep_GoldChanged(int64 Gold) const;
    void Rep_LevelChanged(int32 Level) const;
    void Rep_ExpChanged(TOptional<int32> CurExp, TOptional<int32> MaxExp) const;
    void Rep_StatChanged(const Protocol::StatInfo& InStatInfo) const;
    void Rep_HpChanged(int32 UpdatedHp);

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnMyPlayerSpawned, AP1MyPlayer*);
    FOnMyPlayerSpawned OnMyPlayerSpawned;

protected:
    UPROPERTY()
    UInventory* Inventory;

    UPROPERTY()
    UEquippedGear* EquippedGear;

private:
    uint64 _PlayerId = 0;                           // ObjectId
    FText _PlayerName = FText::FromString("NULL");  // Nickname
    Protocol::PlayerInfo* _PlayerInfo;
};
