#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Protocol.pb.h"
#include "Sync/P1StatefulEntityManager.h"
#include "P1GameInstance.generated.h"

class UP1MyPlayerData;
class UP1ConnectionSubsystem;
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
    virtual void BeginDestroy() override;
    //~ End UGameInstance Interface

    //~ Connection
private:
    /** 게임 서버와 연결되어 있는지다. 연결은 UP1ConnectionSubsystem이 소유한다. */
    bool IsConnected() const;

    UPROPERTY()
    TObjectPtr<UP1ConnectionSubsystem> _Connection;

    //~ Connection Loss
public:
    /** 서버가 S_LEAVE_GAME으로 연결을 끊었다. 사유에 맞는 문구와 함께 로그인 화면으로 돌아간다. 게임 스레드 전용. */
    void HandleLeaveGame(const Protocol::S_LEAVE_GAME& LeaveGamePkt);

    /** 로그인 화면이 한 번 꺼내 보여 준다. 꺼내면 비워진다. 연결이 끊겨 돌아온 것이 아니면 비어 있다. 게임 스레드 전용. */
    FString ConsumeLoginNotice();

private:
    /** 연결 서브시스템이 수신 펌프에서 끊김을 알아챘다. 사유를 모르므로 기본 문구로 돌아간다. */
    void HandleConnectionLost();

    /** 연결을 정리하고 로그인 맵을 연다. 사용자가 게임을 끄는 경로(연결 서브시스템의 Deinitialize)는 이 경로를 타지 않는다. 게임 스레드 전용. */
    void ReturnToLogin(const FString& Notice);

    /** 로그인 맵이 열린 뒤 로그인 화면에 보여 줄 문구다. 게임 인스턴스가 레벨 전환을 건너 들고 간다. */
    FString PendingLoginNotice;

    //~ Session Packet Handlers
public:
    void HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt);
    void HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt);
    void HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt);

    //~ Entity Lookup
public:
    /** 현재 월드에 스폰된 엔티티를 찾아 T로 캐스트한다. 없거나 타입이 다르면 nullptr. */
    template <typename T>
    T* FindEntityAs(uint64 EntityId) const
    {
        UWorld* World = GetWorld();
        if (World == nullptr)
            return nullptr;

        UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>();
        if (StatefulEntityManager == nullptr)
            return nullptr;

        return Cast<T>(StatefulEntityManager->FindEntity(EntityId));
    }

    //~ Entity Packet Handlers
public:
    void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

    void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);
    void HandleDespawnAll();

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

    /**
     * 위 패킷 수신 델리게이트에서 Listener가 붙인 것을 모두 뗀다.
     * 게임 인스턴스는 레벨보다 오래 살기 때문에, 레벨과 함께 사라지는 객체는 사라지기 전에 불러야 한다.
     */
    void RemovePacketListener(const UObject* Listener);

    //~ Combat Packet Handlers
public:
    void HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt);
    void HandleHit(const Protocol::S_HIT& HitPkt);
    void HandleDie(const Protocol::S_DIE& DiePkt);
    void HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt);
    void HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt);

    //~ My Player
public:
    UP1MyPlayerData* GetMyPlayerData();

protected:
    void HandleMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    UPROPERTY()
    TObjectPtr<AP1MyPlayer> _MyPlayer;

    UPROPERTY()
    TObjectPtr<UP1MyPlayerData> _MyPlayerData;
};
