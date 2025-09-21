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
#include "Inventory.h"
#include "EquippedGear.h"

UP1GameInstance::UP1GameInstance()
{
    _PlayerInfo = new Protocol::PlayerInfo();
    _StatInfo = _PlayerInfo->mutable_stat_info();
}

void UP1GameInstance::Init()
{
    Super::Init();

    InventoryHelper = NewObject<UInventory>(this, UInventory::StaticClass());
    EquippedGearHelper = NewObject<UEquippedGear>(this, UEquippedGear::StaticClass());
}

void UP1GameInstance::BeginDestroy()
{
    Super::BeginDestroy();

    delete _PlayerInfo;
    _PlayerInfo = nullptr;
    _StatInfo = nullptr;
}

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

void UP1GameInstance::RepLevel(int32 Level_)
{
    _PlayerInfo->set_level(Level_);
    OnLevelChanged.Broadcast(_PlayerInfo->level());
}

void UP1GameInstance::RepExp(int32 CurExp, int32 MaxExp)
{
    _PlayerInfo->set_cur_exp(CurExp);
    if (MaxExp > 0)
        _PlayerInfo->set_max_exp(MaxExp);

    OnExpChanged.Broadcast(_PlayerInfo->cur_exp(), _PlayerInfo->max_exp());
}

void UP1GameInstance::RepStatInfo(const Protocol::StatInfo& StatInfo_)
{
    _StatInfo->CopyFrom(StatInfo_);
    OnStatInfoChanged.Broadcast(*_StatInfo);
}

void UP1GameInstance::RepGold(int64 Gold)
{
    _PlayerInfo->set_gold(Gold);
    OnGoldChanged.Broadcast(_PlayerInfo->gold());
}

void UP1GameInstance::RepInventorySlot(const Protocol::Slot& Slot_, bool OnUse)
{
    InventoryHelper->SetSlot(Slot_);
    OnInventorySlotChanged.Broadcast(Slot_, OnUse);
}

void UP1GameInstance::RepEquippedGearSlot(const Protocol::Slot& Slot_)
{
    EquippedGearHelper->SetSlot(Slot_);
    OnEquippedGearSlotChanged.Broadcast(Slot_);
}

////////////////////// Replication End //////////////////////////

void UP1GameInstance::HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    if (EnterGamePkt.success() == false)
        return;

    // Init MyPlayer Data
    const Protocol::PlayerInfo& PlayerInfo_ = EnterGamePkt.player().player_info();
    _PlayerInfo->CopyFrom(PlayerInfo_);
    _StatInfo = _PlayerInfo->mutable_stat_info();   // CopyFrom 시, 포인터 주소가 달라질 수 있음.

    _MyPlayerId = EnterGamePkt.player().object_id();
    InventoryHelper->Init(_PlayerInfo->mutable_inventory());
    EquippedGearHelper->Init(_PlayerInfo);
}

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
        AP1Player* Player = Cast<AP1Player>(World->SpawnActor(MyPlayerClass, &SpawnLocation));
        MyPlayer = Player;
        Players.Add(ObjectInfo.object_id(), Player);
        
        Player->Init(ObjectInfo);   // 갑옷 메시 입히는 용
	}
	else
	{
		AP1Player* Player = Cast<AP1Player>(World->SpawnActor(OtherPlayerClass, &SpawnLocation));
        Players.Add(ObjectInfo.object_id(), Player);
		
        Player->Init(ObjectInfo);   // 갑옷 메시 입히는 용
	}
}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (auto& Player : SpawnPkt.objects())
	{
        bool IsMine = Player.object_id() == _MyPlayerId;
		HandleSpawn(Player, IsMine);
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
        OnRep_BuyItem.Broadcast();
        if (BuyItemPkt.success() == true)
        {
            RepInventorySlot(BuyItemPkt.updated_slot());
            RepGold(BuyItemPkt.gold());
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

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        OnRep_SellItem.Broadcast();
        if (SellItemPkt.success() == true)
        {
            RepInventorySlot(SellItemPkt.updated_slot());
            RepGold(SellItemPkt.gold());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);
    if (Player->IsMyPlayer() == false)
        return;

    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(MyPlayer))
    {
        OnRep_UseItem.Broadcast();
        if (UseItemPkt.success() == true)
        {
            auto& Slot = UseItemPkt.updated_inventory_slot();

            if (Slot.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE)
            {
                RepInventorySlot(Slot, true);
                RepStatInfo(UseItemPkt.updated_stat_info());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);
    
    auto& EquippedGearSlot = EquipGearPkt.updated_equipped_slot();
    auto& InvenSlot = EquipGearPkt.updated_inventory_slot();

    // 공통: 장착 부위 매쉬 변경
    {
        const Protocol::Item& Item_ = EquippedGearSlot.item();

        if (Player == MyPlayer)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("MYPLAYERRRRRRRRRRRRRRRRRRRRRRRRRR"));
        }

        // 장착한 갑옷 메시 적용
        Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
    }

    // 내 플레이어: 장비창 + 인벤창 + 스텟 변경
    if (Player->IsMyPlayer())
    {
        OnRep_EquipGear.Broadcast();
        if (EquipGearPkt.success() == true)
        {
            RepEquippedGearSlot(EquippedGearSlot);
            RepInventorySlot(InvenSlot);
            RepStatInfo(EquipGearPkt.updated_stat_info());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);

    auto& EquippedGearSlot = UnequipGearPkt.updated_equipped_slot();
    auto& InvenSlot = UnequipGearPkt.updated_inventory_slot();

    // 장착해서 갱신된 장착 슬롯 정보를 반영.
    {
        const Protocol::Item& Item_ = EquippedGearSlot.item();

        // 장착한 갑옷 메시 적용
        Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
    }

    // 장착해서 갱신된 인벤 슬롯 정보를 반영.
    if (Player->IsMyPlayer())
    {
        OnRep_UnequipGear.Broadcast();
        if (UnequipGearPkt.success() == true)
        {
            RepEquippedGearSlot(EquippedGearSlot);
            RepInventorySlot(InvenSlot);
            RepStatInfo(UnequipGearPkt.updated_stat_info());
        }

    }

}
