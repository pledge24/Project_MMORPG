#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "Utils/Types.h"
#include "Protocol.pb.h"
#include "Sync/P1StatefulObjectManager.h"
#include "P1GameInstance.generated.h"

class UP1MyPlayerData;
class AP1Player;
class AP1MyPlayer;

UCLASS()
class P1_API UP1GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UP1GameInstance();

    //~ Begin UGameInstance Interface
public:
    virtual void Init() override;
    virtual void Shutdown() override;
    virtual void BeginDestroy() override;
    //~ End UGameInstance Interface

    //~ Connection
public:
    UFUNCTION(BlueprintCallable)
    void ConnectToGameServer();

    UFUNCTION(BlueprintCallable)
    void DisconnectFromGameServer();

    class FSocket* Socket;
    const FString IpAddress = TEXT("127.0.0.1");
    const int16 Port = 7777;
    PacketSessionRef GameServerSession;

    //~ Packet Pump
public:
    /** 블루프린트에서 부르지 않는다. Init에서 코어 티커에 등록한 펌프가 유일한 호출자다. */
    void HandleRecvPackets();

    void SendPacket(SendBufferRef SendBuffer);

private:
    /** 코어 티커 콜백이다. true를 돌려주면 다음 프레임에도 호출된다. */
    bool TickRecvPump(float DeltaTime);

    /** Init에서 등록하고 Shutdown에서 해제한다. 게임 인스턴스와 수명이 같다. */
    FTSTicker::FDelegateHandle RecvPumpTickerHandle;

    //~ Session Packet Handlers
public:
    void HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt);
    void HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt);
    void HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt);

    //~ Entity Packet Handlers
public:
    void HandleSpawn(const Protocol::EntityInfo& EntityInfo);
    void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

    void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);
    void HandleDespawnAll(bool ExceptMine = false);

    void HandleMove(const Protocol::PosInfo& Info);
    void HandleMove(const Protocol::S_MOVE& MovePkt);

    //~ Trade Packet Handlers
public:
    void HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt);
    void HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt);
    void HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt);

    DECLARE_MULTICAST_DELEGATE(FOnRecvBuyItemPkt);
    FOnRecvBuyItemPkt OnRecvBuyItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvSellItemPkt);
    FOnRecvSellItemPkt OnRecvSellItemPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUseItemPkt);
    FOnRecvUseItemPkt OnRecvUseItemPkt;

    //~ Equipment Packet Handlers
public:
    void HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt);
    void HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt);

    DECLARE_MULTICAST_DELEGATE(FOnRecvEquipGearPkt);
    FOnRecvEquipGearPkt OnRecvEquipGearPkt;

    DECLARE_MULTICAST_DELEGATE(FOnRecvUnequipGearPkt);
    FOnRecvUnequipGearPkt OnRecvUnequipGearPkt;

    //~ Combat Packet Handlers
public:
    void HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt);
    void HandleHit(const Protocol::S_HIT& HitPkt);
    void HandleDie(const Protocol::S_DIE& DiePkt);
    void HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt);
    void HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt);

    //~ Auth Token
public:
    FString GetToken() const { return _token; }
    void SetToken(FString token) { _token = token; }

    /** 게임 서버 접속에 쓰는 토큰이다. 인증 서버가 발급한다. */
    FString _token = "";

    //~ My Player
public:
    UP1MyPlayerData* GetMyPlayerData();
    void SetMyPlayer(AP1MyPlayer* MyPlayer) { _MyPlayer = MyPlayer; }

protected:
    UPROPERTY()
    TObjectPtr<AP1MyPlayer> _MyPlayer;

    UPROPERTY()
    TObjectPtr<UP1MyPlayerData> _MyPlayerData;
};
