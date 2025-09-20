// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Types.h"
#include "Protocol.pb.h"
#include "P1GameInstance.generated.h"

class AP1Player;
class AP1MyPlayer;

/**
 * 
 */
UCLASS()
class P1_API UP1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    UP1GameInstance();
    virtual void Init() override;
    virtual void BeginDestroy() override;

	/* 네트워크 통신 관련 */
	UFUNCTION(BlueprintCallable)
	void ConnectToGameServer();

	UFUNCTION(BlueprintCallable)
	void DisconnectFromGameServer();

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

	void SendPacket(SendBufferRef SendBuffer);

public:
    /** Getter */
    const Protocol::PlayerInfo& GetPlayerInfo() { return *_PlayerInfo; }
    int32 GetGold() { return _PlayerInfo->gold(); };
    int32 GetLevel() { return _PlayerInfo->level(); };

    /** 레벨 관련 Rep  */
    void RepLevel(int32 Level_);
    void RepExp(int32 CurExp, int32 MaxExp = -1);

    /** 스텟 관련 Rep */
    void RepStatInfo(const Protocol::StatInfo& StatInfo_);

    /** 소유 관련 Rep */
    void RepGold(int64 Gold);
    void RepInventorySlot(const Protocol::Slot& Slot_, bool OnUse = false);
    void RepEquippedGearSlot(const Protocol::Slot& Slot_);

public:
	/* 패킷 핸들 함수 */
	void HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt);

	void HandleSpawn(const Protocol::ObjectInfo& PlayerInfo, bool IsMine);
	void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

	void HandleDespawn(uint64 ObjectId);
	void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);

	void HandleMove(const Protocol::S_MOVE& MovePkt);

    void HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt);
    void HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt);
    void HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt);

    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

public:
    /** 델리게이트 모음(위젯 상태 갱신용) */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLevelChanged, int32);
    FOnLevelChanged OnLevelChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnExpChanged, int32, int32);
    FOnExpChanged OnExpChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnStatInfoChanged, const Protocol::StatInfo&);
    FOnStatInfoChanged OnStatInfoChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, const int32);
    FOnGoldChanged OnGoldChanged;

    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotChanged, const Protocol::Slot&, bool);
    FOnInventorySlotChanged OnInventorySlotChanged;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedGearSlotChanged, const Protocol::Slot&);
    FOnEquippedGearSlotChanged OnEquippedGearSlotChanged;

    /** 델리게이트 모음(위젯 액션 알림용) */
    DECLARE_MULTICAST_DELEGATE(FOnRep_BuyItem);
    FOnRep_BuyItem OnRep_BuyItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_SellItem);
    FOnRep_SellItem OnRep_SellItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_UseItem);
    FOnRep_UseItem OnRep_UseItem;

    DECLARE_MULTICAST_DELEGATE(FOnRep_EquipGear);
    FOnRep_EquipGear OnRep_EquipGear;

    DECLARE_MULTICAST_DELEGATE(FOnRep_UnequipGear);
    FOnRep_UnequipGear OnRep_UnequipGear;

public:
	void SetToken(FString token) { _token = token; }
	FString GetToken() { return _token; }

public:
	/** GameServer Socket */
	class FSocket* Socket;
	FString IpAddress = TEXT("127.0.0.1");
	int16 Port = 7777;
	PacketSessionRef GameServerSession;

public:
	/** Player 정보 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AP1Player> OtherPlayerClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1MyPlayer> MyPlayerClass;

	AP1Player* MyPlayer;
	TMap<uint64, AP1Player*> Players;

    /** MyPlayer 고유 정보 */
    UPROPERTY()
    TObjectPtr<class UInventory> InventoryHelper;

    UPROPERTY()
    TObjectPtr<class UEquippedGear> EquippedGearHelper;

    uint64 _MyPlayerId;
    Protocol::PlayerInfo* _PlayerInfo;
    Protocol::StatInfo* _StatInfo;

private:
	FString _token = "";
};
