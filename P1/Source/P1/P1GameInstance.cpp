// Fill out your copyright notice in the Description page of Project Settings.

#include "P1GameInstance.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "P1MyPlayer.h"
#include "P1.h"

void UP1GameInstance::ConnectToGameServer()
{
	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connecting To Server...")));

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Success")));

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
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Failed")));
	}
}

void UP1GameInstance::DisconnectFromGameServer()
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	SEND_PACKET(LeavePkt);

	//if (Socket)
	//{
	//	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
	//	SocketSubsystem->DestroySocket(Socket);
	//	Socket = nullptr;
	//}
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

//////////////////////Network End//////////////////////////

// 새로운 오브젝트(본인 포함)를 맵에 스폰
void UP1GameInstance::HandleSpawn(const Protocol::ObjectInfo& ObjectInfo, bool IsMine)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	// 있으면 안된다.
	const uint64 ObjectId = ObjectInfo.object_id();
	if (Players.Find(ObjectId) != nullptr)
		return;

	FVector SpawnLocation(ObjectInfo.pos_info().x(), ObjectInfo.pos_info().y(), ObjectInfo.pos_info().z());

	if (IsMine)
	{
        PendingMyPlayerData = std::move(ObjectInfo);
        bHasPendingMyPlayer = true;
	}
	else
	{
		AP1Player* Player = Cast<AP1Player>(World->SpawnActor(OtherPlayerClass, &SpawnLocation));
		Player->Init(ObjectInfo);
		Players.Add(ObjectInfo.object_id(), Player);
	}
}

void UP1GameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	HandleSpawn(EnterGamePkt.player(), true);
}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (auto& Player : SpawnPkt.objects())
	{
		HandleSpawn(Player, false);
	}
}

void UP1GameInstance::HandleDespawn(uint64 ObjectId)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	AP1Player** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	World->DestroyActor(*FindActor);
}

void UP1GameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
	for (auto& ObjectId : DespawnPkt.object_ids())
	{
		HandleDespawn(ObjectId);
	}
}

void UP1GameInstance::HandleMove(const Protocol::S_MOVE& MovePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = MovePkt.info().object_id();
	AP1Player** FindActor = Players.Find(ObjectId);
	if (FindActor == nullptr)
		return;

	AP1Player* Player = (*FindActor);
	if (Player->IsMyPlayer())
		return;

	const Protocol::PosInfo& Info = MovePkt.info();
	//Player->SetPlayerInfo(Info);
	Player->SetDestInfo(Info);
}

void UP1GameInstance::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        MyPlayer_->OnAddItemDelegate.Broadcast(BuyItemPkt.updated_slot());
        MyPlayer_->OnChangedGoldDelegate.Broadcast(BuyItemPkt.gold());
    }
}

void UP1GameInstance::HandleSellItem(const Protocol::S_SELL_ITEM& SellItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        MyPlayer_->OnRemoveItemDelegate.Broadcast(SellItemPkt.updated_slot());
        MyPlayer_->OnChangedGoldDelegate.Broadcast(SellItemPkt.gold());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        for (auto& Slot : EquipGearPkt.updated_slots())
        {
            if (Slot.type() == Protocol::SlotType::SLOT_TYPE_EQUIPPED)
            {
                MyPlayer_->OnEquipGearDelegate.Broadcast(Slot);
            }
            else
            {
                MyPlayer_->OnRemoveItemDelegate.Broadcast(Slot);
            }
        }

        MyPlayer_->OnChangedStatInfoDelegate.Broadcast(EquipGearPkt.updated_stat_info());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        for (auto& Slot : UnequipGearPkt.updated_slots())
        {
            if (Slot.type() == Protocol::SlotType::SLOT_TYPE_EQUIPPED)
            {
                MyPlayer_->OnUnequipGearDelegate.Broadcast(Slot);
            }
            else
            {
                MyPlayer_->OnAddItemDelegate.Broadcast(Slot);
            }
        }

        MyPlayer_->OnChangedStatInfoDelegate.Broadcast(UnequipGearPkt.updated_stat_info());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);
    if (Player->IsMyPlayer() == false)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        for (auto& Slot : UseItemPkt.updated_slots())
        {
            if (Slot.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE)
            {
                MyPlayer_->OnRemoveItemDelegate.Broadcast(Slot);
            }
        }

        MyPlayer_->OnChangedStatInfoDelegate.Broadcast(UseItemPkt.updated_stat_info());
    }
}
