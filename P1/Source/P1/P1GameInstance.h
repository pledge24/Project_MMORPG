// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Types.h"
#include "Protocol.pb.h"
#include "P1GameInstance.generated.h"

class UMyPlayerData;
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
    virtual void Shutdown() override;
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
	/* 패킷 핸들 함수 */
	void HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt);

	void HandleSpawn(const Protocol::ObjectInfo& PlayerInfo, bool IsMine);
	void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

	void HandleDespawn(uint64 ObjectId);
	void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);
    void HandleDespawnAll(bool ExceptMine = false);

    void HandleMove(const Protocol::PosInfo& Info);
	void HandleMove(const Protocol::S_MOVE& MovePkt);

    void HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt);
    void HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt);
    void HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt);

    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

    void HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt);

    /** Getter 함수 */
    FString GetToken() const { return _token; }
    UMyPlayerData* GetMyPlayerData() const { return _MyPlayerData; }

    /** Setter 함수 */
    void SetToken(FString token) { _token = token; }

public:
	/** Player 정보 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AP1Player> OtherPlayerClass;

    UPROPERTY(EditAnywhere)
    TSubclassOf<AP1MyPlayer> MyPlayerClass;

	AP1MyPlayer* MyPlayer;
	TMap<uint64, AP1Player*> Players;

    /** GameServer Socket */
    class FSocket* Socket;
    FString IpAddress = TEXT("127.0.0.1");
    int16 Port = 7777;
    PacketSessionRef GameServerSession;

protected:
    UPROPERTY()
    UMyPlayerData* _MyPlayerData;

	FString _token = ""; // GameServer 접속 토큰
};
