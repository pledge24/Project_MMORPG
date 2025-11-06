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
        UE_LOG(LogTemp, Warning, TEXT("_MyPlayerData Is Not Stored"));
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
    FRotator SpawnRotator(0.f, ObjectInfo.pos_info().yaw(), 0.f);


	if (IsMine)
	{
        // 리스폰할때 진입
        if (IsValid(MyPlayer) == true)
        {
            // bool bRespawn = false;
            MyPlayer->PushToMoveQueue(ObjectInfo.pos_info());
            return;
        }
        
        // Map에 처음 진입했을때 진입
        AP1Player* Player = World->SpawnActor<AP1Player>(MyPlayerClass, SpawnLocation, SpawnRotator);
        MyPlayer = Cast<AP1MyPlayer>(Player);
        Players.Add(ObjectInfo.object_id(), Player);
    
        Player->Initialize(ObjectInfo);   // 갑옷 메시 입히는 용
	}
	else
	{
        // Set Spawn Point
        AP1Player* Player = World->SpawnActor<AP1Player>(OtherPlayerClass, SpawnLocation, SpawnRotator);
        Players.Add(ObjectInfo.object_id(), Player);
		
        Player->Initialize(ObjectInfo);   // 갑옷 메시 입히는 용
	}
}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
    UMyPlayerData* MyPlayerData = GetSubsystem<UMyPlayerData>();
    uint64 MyPlayerId = MyPlayerData->GetPlayerId();

	for (auto& Player : SpawnPkt.objects())
	{
        bool IsMine = Player.object_id() == MyPlayerId;
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

    if (Players.Remove(ObjectId) > 0)
    {
	    World->DestroyActor(*FindActor);
    }
}

void UP1GameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
	for (auto& ObjectId : DespawnPkt.object_ids())
	{
		HandleDespawn(ObjectId);
	}
}

void UP1GameInstance::HandleDespawnAll(bool ExceptMine)
{
    uint64 MyPlayerId = _MyPlayerData->GetPlayerId();

    uint64 ExceptId = ExceptMine ? MyPlayerId : 0;
    for (auto Item : Players)
    {
        if (ExceptId != Item.Key)
            HandleDespawn(Item.Key);
    }
}

void UP1GameInstance::HandleMove(const Protocol::PosInfo& Info)
{
    AP1Player** FindActor = Players.Find(Info.object_id());
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);
    Player->PushToMoveQueue(Info);
}

void UP1GameInstance::HandleMove(const Protocol::S_MOVE& MovePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

    HandleMove(MovePkt.info());
}

void UP1GameInstance::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (IsValid(MyPlayer) == true)
    {
        MyPlayer->OnRecvBuyItemPkt.Broadcast();
        if (BuyItemPkt.success() == true)
        {
            MyPlayer->OnInvenSlotChanged.Broadcast(BuyItemPkt.updated_slot(), false);
            MyPlayer->OnGoldChanged.Broadcast(BuyItemPkt.gold());
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

    if (IsValid(MyPlayer) == true)
    {
        MyPlayer->OnRecvSellItemPkt.Broadcast();
        if (SellItemPkt.success() == true)
        {
            MyPlayer->OnInvenSlotChanged.Broadcast(SellItemPkt.updated_slot(), false);
            MyPlayer->OnGoldChanged.Broadcast(SellItemPkt.gold());
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

    if (IsValid(MyPlayer) == true)
    {
        MyPlayer->OnRecvUseItemPkt.Broadcast();
        if (UseItemPkt.success() == true)
        {
            auto& Slot = UseItemPkt.updated_inventory_slot();
            MyPlayer->OnInvenSlotChanged.Broadcast(Slot, true);
            MyPlayer->OnStatInfoChanged.Broadcast(UseItemPkt.updated_stat_info());
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
    if(EquipGearPkt.success() == true){
        const Protocol::Item& Item_ = EquippedGearSlot.item();

        // 장착한 갑옷 메시 적용
        Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
    }

    // 내 플레이어: 장비창 + 인벤창 + 스텟 변경
    if (Player->IsMyPlayer())
    {
        MyPlayer->OnRecvEquipGearPkt.Broadcast();
        if (EquipGearPkt.success() == true)
        {
            MyPlayer->OnGearSlotChanged.Broadcast(EquippedGearSlot);
            MyPlayer->OnInvenSlotChanged.Broadcast(InvenSlot, false);
            MyPlayer->OnStatInfoChanged.Broadcast(EquipGearPkt.updated_stat_info());
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
    if (UnequipGearPkt.success() == true)
    {
        const Protocol::Item& Item_ = EquippedGearSlot.item();

        // 장착한 갑옷 메시 적용
        Player->ChangeMesh(EquippedGearSlot.slot_id(), Item_.template_id());
    }

    // 장착해서 갱신된 인벤 슬롯 정보를 반영.
    if (Player->IsMyPlayer())
    {
        MyPlayer->OnRecvUnequipGearPkt.Broadcast();
        if (UnequipGearPkt.success() == true)
        {
            MyPlayer->OnGearSlotChanged.Broadcast(EquippedGearSlot);
            MyPlayer->OnInvenSlotChanged.Broadcast(InvenSlot, false);
            MyPlayer->OnStatInfoChanged.Broadcast(UnequipGearPkt.updated_stat_info());
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
    AP1Player** FindActor = Players.Find(ObjectId);
    if (FindActor == nullptr)
        return;

    AP1Player* Player = (*FindActor);
    uint32 Combo = NormalAttackPkt.combo();

    Player->S_NormalAttack(Combo);
}
