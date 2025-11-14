// Fill out your copyright notice in the Description page of Project Settings.

#include "P1GameInstance.h"

#include "AttackSystemComponent.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "Objects/P1MyPlayer.h"
#include "P1.h"
#include "Creature.h"
#include "MyPlayerData.h"
#include "Log/LogCategory.h"
#include "P1MyPlayer.h"

UP1GameInstance::UP1GameInstance()
{
}

void UP1GameInstance::Init()
{
    Super::Init();

    _MyPlayerData = GetSubsystem<UMyPlayerData>();
    if (IsValid(_MyPlayerData) == false)
        UE_LOG(LogTemp, Warning, TEXT("_MyPlayerData Is Invalid"));
}

void UP1GameInstance::Shutdown()
{
    Super::Shutdown();

    // 게임 서버 연결 해제
    DisconnectFromGameServer();
}

void UP1GameInstance::BeginDestroy()
{
    Super::BeginDestroy();
}


/*-------------------------
*       Network Method
 -------------------------*/
#pragma region Network Method

void UP1GameInstance::ConnectToGameServer()
{
	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Success To Connect GameServer")));

		// Session
		GameServerSession = MakeShared<PacketSession>(Socket);
		GameServerSession->Run();

		// AuthServer로부터 받은 AccessToken과 함께 로그인 패킷 전송
		{
			Protocol::C_LOGIN Pkt;
			Pkt.set_access_token(TCHAR_TO_UTF8(*_token));

			SendBufferRef SendBuffer = ClientPacketHandler::MakeSerializedPacket(Pkt);
			SendPacket(SendBuffer);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("Fail To Connect GameServer")));
	}
}

void UP1GameInstance::DisconnectFromGameServer()
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	SEND_PACKET(LeavePkt);
}

void UP1GameInstance::HandleRecvPackets()
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->HandleRecvPackets();
}

void UP1GameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->SendPacket(SendBuffer);
}

#pragma endregion Network Method


/*-------------------------
 *  Handle Packet Method
 -------------------------*/

void UP1GameInstance::HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    if (EnterGamePkt.success() == false)
        return;

    UMyPlayerData* MyPlayerData = GetSubsystem<UMyPlayerData>();
    const Protocol::ObjectInfo& ObjectInfo = EnterGamePkt.player();

    // 게임 서버에 입장한 시점에 가져온 캐릭터의 모든 정보를 저장한다.
    MyPlayerData->InitMyPlayerData(ObjectInfo);
}

void UP1GameInstance::HandleSpawn(const Protocol::ObjectInfo& ObjectInfo)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        StatefulObjectManager->SpawnObject(ObjectInfo);
    }
}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
	    for (auto& Object : SpawnPkt.objects())
	    {
            StatefulObjectManager->SpawnObject(Object);
	    }
    }
}

void UP1GameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        for (auto& ObjectId : DespawnPkt.object_ids())
        {
            StatefulObjectManager->DespawnObject(ObjectId);
        }
    }
	
}

void UP1GameInstance::HandleDespawnAll(bool ExceptMine)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        StatefulObjectManager->DespawnAllObjects(ExceptMine);
    }
}

void UP1GameInstance::HandleMove(const Protocol::PosInfo& Info)
{
    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(Info.object_id());
        if (FindActor == nullptr)
            return;

        if (ACreature* Creature = Cast<ACreature>(FindActor))
        {
            Creature->PushToMoveQueue(Info);
        }
    }
}

void UP1GameInstance::HandleMove(const Protocol::S_MOVE& MovePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	//auto* World = GetWorld();
	//if (World == nullptr)
	//	return;

    HandleMove(MovePkt.info());
}

void UP1GameInstance::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (IsValid(_MyPlayer) == true)
    {
        _MyPlayer->OnRecvBuyItemPkt.Broadcast();
        if (BuyItemPkt.success() == true)
        {
            _MyPlayer->OnInvenSlotChanged.Broadcast(BuyItemPkt.updated_slot(), false);
            _MyPlayer->OnGoldChanged.Broadcast(BuyItemPkt.gold());
        }
    }
}

void UP1GameInstance::HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (IsValid(_MyPlayer) == true)
    {
        _MyPlayer->OnRecvSellItemPkt.Broadcast();
        if (SellItemPkt.success() == true)
        {
            _MyPlayer->OnInvenSlotChanged.Broadcast(SellItemPkt.updated_slot(), false);
            _MyPlayer->OnGoldChanged.Broadcast(SellItemPkt.gold());
        }
    }
}

void UP1GameInstance::HandleUseItem(const Protocol::S_USE_ITEM& UseItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    const uint64 ObjectId = UseItemPkt.object_id();
    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        if (Player->IsMyPlayer() == false)
            return;

        if (IsValid(_MyPlayer) == true)
        {
            _MyPlayer->OnRecvUseItemPkt.Broadcast();
            if (UseItemPkt.success() == true)
            {
                auto& Slot = UseItemPkt.updated_inventory_slot();
                _MyPlayer->OnInvenSlotChanged.Broadcast(Slot, true);
                _MyPlayer->OnStatInfoChanged.Broadcast(UseItemPkt.updated_stat_info());
            }
        }
    }

}

void UP1GameInstance::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    const uint64 ObjectId = EquipGearPkt.object_id();
    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        auto& EquippedGearSlot = EquipGearPkt.updated_equipped_slot();
        auto& InvenSlot = EquipGearPkt.updated_inventory_slot();

        // 공통: 장착 부위 매쉬 변경
        if(EquipGearPkt.success() == true){
            const Protocol::Item& Item_ = EquippedGearSlot.item();

            // 장착한 갑옷 메시 적용
            Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
        }

        // 내 플레이어: 장비창 + 인벤창 + 스텟 변경
        if (Player->IsMyPlayer())
        {
            _MyPlayer->OnRecvEquipGearPkt.Broadcast();
            if (EquipGearPkt.success() == true)
            {
                _MyPlayer->OnGearSlotChanged.Broadcast(EquippedGearSlot);
                _MyPlayer->OnInvenSlotChanged.Broadcast(InvenSlot, false);
                _MyPlayer->OnStatInfoChanged.Broadcast(EquipGearPkt.updated_stat_info());
            }
        }
    }

}

void UP1GameInstance::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    const uint64 ObjectId = UnequipGearPkt.object_id();
    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        auto& EquippedGearSlot = UnequipGearPkt.updated_equipped_slot();
        auto& InvenSlot = UnequipGearPkt.updated_inventory_slot();

        // 장착해서 갱신된 장착 슬롯 정보를 반영.
        if (UnequipGearPkt.success() == true)
        {
            const Protocol::Item& Item_ = EquippedGearSlot.item();

            // 장착한 갑옷 메시 적용
            Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
        }

        // 장착해서 갱신된 인벤 슬롯 정보를 반영.
        if (Player->IsMyPlayer())
        {
            _MyPlayer->OnRecvUnequipGearPkt.Broadcast();
            if (UnequipGearPkt.success() == true)
            {
                _MyPlayer->OnGearSlotChanged.Broadcast(EquippedGearSlot);
                _MyPlayer->OnInvenSlotChanged.Broadcast(InvenSlot, false);
                _MyPlayer->OnStatInfoChanged.Broadcast(UnequipGearPkt.updated_stat_info());
            }

        }

    }

}

void UP1GameInstance::HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    const uint64 ObjectId = NormalAttackPkt.object_id();
    if (UStatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UStatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        uint32 Combo = NormalAttackPkt.combo();

        Player->S_NormalAttack(Combo);
    }

}

UMyPlayerData* UP1GameInstance::GetMyPlayerData()
{
    if (IsValid(_MyPlayerData) == false)
        _MyPlayerData = GetSubsystem<UMyPlayerData>();

    return _MyPlayerData;
}
