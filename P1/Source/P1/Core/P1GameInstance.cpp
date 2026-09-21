#include "Core/P1GameInstance.h"

#include "Combat/P1AttackSystemComponent.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Network/PacketSession.h"
#include "Protocol.pb.h"
#include "Network/ClientPacketHandler.h"
#include "Characters/P1MyPlayer.h"
#include "P1.h"
#include "Characters/P1Creature.h"
#include "Core/P1MyPlayerData.h"
#include "Utils/LogCategory.h"
#include "Characters/P1MyPlayer.h"

UP1GameInstance::UP1GameInstance()
{
}

void UP1GameInstance::Init()
{
    Super::Init();

    _MyPlayerData = GetSubsystem<UP1MyPlayerData>();
    if (IsValid(_MyPlayerData) == false)
        UE_LOG(LogP1System, Warning, TEXT("_MyPlayerData Is Invalid"));

    // 수신 펌프를 코어 티커에 등록한다.
    // 게임 인스턴스는 레벨 전환에 살아남으므로 펌프도 레벨과 무관하게 계속 돈다.
    // 코어 티커는 게임 스레드에서 돌기 때문에 여기서 UObject를 만져도 된다.
    RecvPumpTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UP1GameInstance::TickRecvPump));
}

void UP1GameInstance::Shutdown()
{
    // 펌프를 먼저 멈춘다. 종료 중에 패킷을 처리하면 이미 정리된 오브젝트를 건드린다.
    if (RecvPumpTickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(RecvPumpTickerHandle);
        RecvPumpTickerHandle.Reset();
    }

    Super::Shutdown();

    // 게임 서버 연결 해제
    DisconnectFromGameServer();
}

bool UP1GameInstance::TickRecvPump(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
        return true;

    // 월드가 준비되기 전과 정리 중에는 펌프를 돌리지 않는다.
    // 레벨 블루프린트 틱은 레벨 전환 동안 죽어 있어서 그 사이 패킷이 큐에 쌓였다가
    // 월드가 준비된 뒤 처리됐다. 코어 티커는 전환 중에도 돌기 때문에 그 버퍼링을
    // 여기서 직접 복원한다. 큐는 비우지 않으므로 패킷은 유실되지 않는다.
    if (World->bIsTearingDown || World->HasBegunPlay() == false)
        return true;

    // 코어 티커는 월드 틱 밖에서 돌기 때문에 이 시점의 GWorld는 게임 월드가 아니다.
    // 에디터에서는 에디터 월드를 가리키고, 그 월드에는 게임 인스턴스가 없다.
    // 패킷 핸들러 20개가 GWorld->GetGameInstance()로 시작하므로 호출 구간에만 맞춰 준다.
    UWorld* PrevWorld = GWorld;
    GWorld = World;
    HandleRecvPackets();
    GWorld = PrevWorld;

    return true;
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

    UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>();
    const Protocol::ObjectInfo& ObjectInfo = EnterGamePkt.player();

    // 게임 서버에 입장한 시점에 가져온 캐릭터의 모든 정보를 저장한다.
    MyPlayerData->InitMyPlayerData(EnterGamePkt);

    // TEMP
    UGameplayStatics::OpenLevel(GWorld, FName("InGameMap"));
}

void UP1GameInstance::HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt)
{
    if (EnterMapPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("맵 입장에 실패했습니다. map_id: %d"), EnterMapPkt.map_id());
        return;
    }

    // 입장한 map + room 정보 저장
    if(UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>())
    {
        MyPlayerData->SetRoomId(EnterMapPkt.room_id());
        MyPlayerData->SetMapId(EnterMapPkt.map_id());
    }

    // TEMP
    UGameplayStatics::OpenLevel(GWorld, FName("InGameMap"));
}

void UP1GameInstance::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("Room 입장에 실패했습니다. room_id: %d"), EnterRoomPkt.room_id());
        return;
    }

    if (UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>())
    {
        MyPlayerData->SetRoomId(EnterRoomPkt.room_id());

        // 단순 방 이동이라면 나를 제외한 모든 오브젝트를 Despawn + 텔레포트
        if (EnterRoomPkt.enter_type() == Protocol::ENTER_TYPE_SAME_MAP_TRANSFER)
        {
            HandleDespawnAll(true);
            if (EnterRoomPkt.has_enter_pos() && IsValid(_MyPlayer))
            {
                _MyPlayer->SetClientPos(EnterRoomPkt.enter_pos());
                _MyPlayer->SetServerPos(EnterRoomPkt.enter_pos());
            }
        }
        
    }

}

void UP1GameInstance::HandleSpawn(const Protocol::ObjectInfo& ObjectInfo)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
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

    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
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

    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
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

    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        StatefulObjectManager->DespawnAllObjects(ExceptMine);
    }
}

void UP1GameInstance::HandleMove(const Protocol::PosInfo& Info)
{
    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(Info.object_id());
        if (FindActor == nullptr)
            return;

        if (AP1Creature* Creature = Cast<AP1Creature>(FindActor))
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

    for(auto& info : MovePkt.info())
        HandleMove(info);
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
        OnRecvBuyItemPkt.Broadcast();
        if (BuyItemPkt.success() == true)
        {
            _MyPlayerData->OnInvenSlotChanged.Broadcast(BuyItemPkt.updated_slot(), false);
            _MyPlayerData->OnGoldChanged.Broadcast(BuyItemPkt.gold());
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
        OnRecvSellItemPkt.Broadcast();
        if (SellItemPkt.success() == true)
        {
            _MyPlayerData->OnInvenSlotChanged.Broadcast(SellItemPkt.updated_slot(), false);
            _MyPlayerData->OnGoldChanged.Broadcast(SellItemPkt.gold());
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
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
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
            OnRecvUseItemPkt.Broadcast();
            if (UseItemPkt.success() == true)
            {
                for (auto& Slot_ : UseItemPkt.updated_slots())
                {
                    _MyPlayerData->OnInvenSlotChanged.Broadcast(Slot_, true);
                }

                for (const auto& Stat_ : UseItemPkt.updated_stat())
                {
                    FOnStatChanged& OnThisStatChanged = _MyPlayerData->OnStatChangedMappings[Stat_.type()];
                    OnThisStatChanged.Broadcast(Stat_.value());
                }
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
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        // 공통: 장착 부위 매쉬 변경
        if(EquipGearPkt.success() == true){
            int32 SlotId = EquipGearPkt.slot_id();
            int32 TemplateId = EquipGearPkt.template_id();

            // 장착한 갑옷 메시 적용
            Player->ChangeMesh(SlotId, TemplateId);
        }

        // 내 플레이어: 장비창 + 인벤창 + 스텟 변경
        if (Player->IsMyPlayer())
        {
            OnRecvEquipGearPkt.Broadcast();
            if (EquipGearPkt.success() == true)
            {
                for (auto& Slot_ : EquipGearPkt.updated_slots())
                {
                    switch (Slot_.type())
                    {
                    case Protocol::SLOT_TYPE_EQUIPPED:
                    {
                        _MyPlayerData->OnEquipmentSlotChanged.Broadcast(Slot_);
                        break;
                    }
                    case Protocol::SLOT_TYPE_INVENTORY_GEAR:
                    case Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE:
                    case Protocol::SLOT_TYPE_INVENTORY_MISC:
                    {
                        _MyPlayerData->OnInvenSlotChanged.Broadcast(Slot_, false);
                        break;
                    }
                    }
                }

                for (auto& Stat_ : EquipGearPkt.updated_stat())
                {
                    FOnStatChanged OnThisStatChanged = _MyPlayerData->OnStatChangedMappings[Stat_.type()];
                    OnThisStatChanged.Broadcast(Stat_.value());
                }

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
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Player* Player = Cast<AP1Player>(FindActor);
        if (Player == nullptr)
            return;

        // 공통: 장착 부위 매쉬 변경
        if (UnequipGearPkt.success() == true)
        {
            int32 SlotId = UnequipGearPkt.slot_id();
            int32 TemplateId = UnequipGearPkt.template_id();

            // 장착한 갑옷 메시 적용
            Player->ChangeMesh(SlotId, TemplateId);
        }

        // 장착해서 갱신된 인벤 슬롯 정보를 반영.
        if (Player->IsMyPlayer())
        {
            OnRecvUnequipGearPkt.Broadcast();
            if (UnequipGearPkt.success() == true)
            {
                for (auto& Slot_ : UnequipGearPkt.updated_slots())
                {
                    switch (Slot_.type())
                    {
                    case Protocol::SLOT_TYPE_EQUIPPED:
                    {
                        _MyPlayerData->OnEquipmentSlotChanged.Broadcast(Slot_);
                        break;
                    }
                    case Protocol::SLOT_TYPE_INVENTORY_GEAR:
                    case Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE:
                    case Protocol::SLOT_TYPE_INVENTORY_MISC:
                    {
                        _MyPlayerData->OnInvenSlotChanged.Broadcast(Slot_, false);
                        break;
                    }
                    }
                }

                for (auto& Stat_ : UnequipGearPkt.updated_stat())
                {
                    FOnStatChanged OnThisStatChanged = _MyPlayerData->OnStatChangedMappings[Stat_.type()];
                    OnThisStatChanged.Broadcast(Stat_.value());
                }

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
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;
        
        AP1Creature* Creature = Cast<AP1Creature>(FindActor);
        if (Creature == nullptr)
            return;

        uint32 Combo = NormalAttackPkt.combo();
        float Yaw = NormalAttackPkt.yaw();

        Creature->S_NormalAttack(Combo, Yaw);
    }

}

void UP1GameInstance::HandleHit(const Protocol::S_HIT& HitPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    int64 ObjectId = HitPkt.object_id();
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Creature* Creature = Cast<AP1Creature>(FindActor);
        if (Creature == nullptr)
            return;

        // S_Hit?
        Creature->S_Hit(HitPkt.damage(), HitPkt.updated_hp());

        if (Creature->IsMyPlayer())
        {
            FOnStatChanged OnThisStatChanged = _MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_HP];
            OnThisStatChanged.Broadcast(HitPkt.updated_hp());
        }
    }
}

void UP1GameInstance::HandleDie(const Protocol::S_DIE& DiePkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    const uint64 ObjectId = DiePkt.object_id();
    if (UP1StatefulObjectManager* StatefulObjectManager = World->GetSubsystem<UP1StatefulObjectManager>())
    {
        AActor* FindActor = StatefulObjectManager->FindObject(ObjectId);
        if (FindActor == nullptr)
            return;

        AP1Creature* Creature = Cast<AP1Creature>(FindActor);
        if (Creature == nullptr)
            return;

        Creature->S_Die();
    }
}

void UP1GameInstance::HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    

}

void UP1GameInstance::HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (RespawnPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("서버에서 리스폰 실패"));
        return;
    }

    UE_LOG(LogP1Network, Warning, TEXT("서버에서 리스폰 성공!"));
}

UP1MyPlayerData* UP1GameInstance::GetMyPlayerData()
{
    if (IsValid(_MyPlayerData) == false)
        _MyPlayerData = GetSubsystem<UP1MyPlayerData>();

    return _MyPlayerData;
}
