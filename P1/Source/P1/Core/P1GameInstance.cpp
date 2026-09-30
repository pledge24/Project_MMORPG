#include "Core/P1GameInstance.h"

#include "Game/Combat/P1AttackSystemComponent.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Network/PacketSession.h"
#include "Network/P1NetworkSettings.h"
#include "Protocol.pb.h"
#include "Network/ClientPacketHandler.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1.h"
#include "Network/P1PacketSender.h"
#include "Game/Entities/P1Creature.h"
#include "Core/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 사유를 모르는 끊김에 보여 줄 문구다. 서버가 먼저 내려갔거나 사유 패킷을 잃은 경우다. */
    const TCHAR* const CONNECTION_LOST_NOTICE = TEXT("게임 서버와 연결이 끊겼습니다.");
}

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
    // 펌프를 먼저 멈춘다. 종료 중에 패킷을 처리하면 이미 정리된 UObject를 건드린다.
    if (RecvPumpTickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(RecvPumpTickerHandle);
        RecvPumpTickerHandle.Reset();
    }

    Super::Shutdown();

    // 게임 서버 연결 해제
    DisconnectFromGameServer();
    CloseGameServerConnection();
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
    // 패킷 핸들러는 GWorld를 보지 않고 세션이 들고 있는 게임 인스턴스를 쓴다.
    HandleRecvPackets();

    return true;
}

void UP1GameInstance::BeginDestroy()
{
    Super::BeginDestroy();
}

//~ Network Method
#pragma region Network Method

void UP1GameInstance::ConnectToGameServer()
{
	// 로그인을 다시 누르면 이 함수가 또 불린다. 이전 연결을 정리하지 않으면 소켓이 샌다.
	CloseGameServerConnection();

	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

	const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();

	FIPv4Address Ip;
	FIPv4Address::Parse(NetworkSettings->GameServerIp, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(NetworkSettings->GameServerPort);

	bool Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Success To Connect GameServer")));

		// Session
		GameServerSession = MakeShared<PacketSession>(Socket, this);
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

		CloseGameServerConnection();
	}
}

void UP1GameInstance::CloseGameServerConnection()
{
	// 세션이 송신 큐를 비운 뒤 소켓을 닫고 수신 스레드를 끝낸다. 순서는 PacketSession::Disconnect에 있다.
	if (GameServerSession)
	{
		GameServerSession->Disconnect();
		GameServerSession = nullptr;
	}

	if (Socket)
	{
		// 연결에 실패해 세션이 없었으면 여기서 처음 닫힌다. 세션이 이미 닫았으면 두 번째 Close는 무시된다.
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
}

void UP1GameInstance::DisconnectFromGameServer()
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	FP1PacketSender::Send(this, LeavePkt);
}

void UP1GameInstance::RemovePacketListener(const UObject* Listener)
{
    OnRecvBuyItemPkt.RemoveAll(Listener);
    OnRecvSellItemPkt.RemoveAll(Listener);
    OnRecvUseItemPkt.RemoveAll(Listener);
    OnRecvEquipGearPkt.RemoveAll(Listener);
    OnRecvUnequipGearPkt.RemoveAll(Listener);
}

void UP1GameInstance::HandleRecvPackets()
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	// 끊김 표시를 큐보다 먼저 읽는다. 수신 워커는 마지막 패킷을 큐에 넣은 뒤 표시를 세우므로,
	// 표시를 본 뒤에 큐를 비우면 끊기기 직전에 온 S_LEAVE_GAME까지 처리한다.
	const bool bConnectionLost = GameServerSession->IsConnectionLost();

	GameServerSession->HandleRecvPackets();

	// S_LEAVE_GAME을 처리했으면 핸들러가 이미 로그인 화면으로 돌려보내 세션이 없다.
	if (bConnectionLost && GameServerSession)
		ReturnToLogin(CONNECTION_LOST_NOTICE);
}

void UP1GameInstance::HandleLeaveGame(const Protocol::S_LEAVE_GAME& LeaveGamePkt)
{
	switch (LeaveGamePkt.reason())
	{
	case Protocol::LEAVE_REASON_DUPLICATE_LOGIN:
		ReturnToLogin(TEXT("다른 곳에서 같은 계정으로 로그인해 연결이 끊겼습니다."));
		break;
	case Protocol::LEAVE_REASON_INVALID_TOKEN:
		ReturnToLogin(TEXT("로그인 정보가 만료되었습니다. 다시 로그인하세요."));
		break;
	default:
		ReturnToLogin(CONNECTION_LOST_NOTICE);
		break;
	}
}

FString UP1GameInstance::ConsumeLoginNotice()
{
	return MoveTemp(PendingLoginNotice);
}

void UP1GameInstance::ReturnToLogin(const FString& Notice)
{
	UE_LOG(LogP1Network, Warning, TEXT("게임 서버 연결이 끊겨 로그인 화면으로 돌아간다: %s"), *Notice);

	CloseGameServerConnection();

	// 토큰은 게임 서버가 한 번 쓰고 지웠다. 다시 들어가려면 인증 서버에 다시 로그인해야 한다.
	_token.Empty();
	_MyPlayer = nullptr;

	PendingLoginNotice = Notice;
	UGameplayStatics::OpenLevel(GetWorld(), FName("L_LoginMap"));
}

void UP1GameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->SendPacket(SendBuffer);
}

#pragma endregion Network Method

//~ Handle Packet Method

void UP1GameInstance::HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    if (EnterGamePkt.success() == false)
        return;

    UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>();
    const Protocol::EntityInfo& EntityInfo = EnterGamePkt.player();

    // 게임 서버에 입장한 시점에 가져온 캐릭터의 모든 정보를 저장한다.
    MyPlayerData->InitMyPlayerData(EnterGamePkt);

    // TEMP
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_InGameMap"));
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
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_InGameMap"));
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

        // 같은 맵 안의 방 이동이라면 나를 제외한 모든 엔티티를 Despawn + 텔레포트.
        // 다른 룸으로 리스폰하는 경우도 룸이 한 맵 안의 논리 분할이라 같은 처리다.
        if (EnterRoomPkt.enter_type() == Protocol::ENTER_TYPE_SAME_MAP_TRANSFER
            || EnterRoomPkt.enter_type() == Protocol::ENTER_TYPE_RESPAWN)
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

void UP1GameInstance::HandleSpawn(const Protocol::EntityInfo& EntityInfo)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>())
    {
        StatefulEntityManager->SpawnEntity(EntityInfo);
    }
}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>())
    {
	    for (auto& Entity : SpawnPkt.entities())
	    {
            StatefulEntityManager->SpawnEntity(Entity);
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

    if (UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>())
    {
        for (auto& EntityId : DespawnPkt.entity_ids())
        {
            StatefulEntityManager->DespawnEntity(EntityId);
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

    if (UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>())
    {
        StatefulEntityManager->DespawnAllEntities(ExceptMine);
    }
}

void UP1GameInstance::HandleMove(const Protocol::PosInfo& Info)
{
    if (AP1Creature* Creature = FindEntityAs<AP1Creature>(Info.entity_id()))
    {
        Creature->PushToMoveQueue(Info);
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
            for (const Protocol::Slot& UpdatedSlot : BuyItemPkt.updated_slots())
            {
                _MyPlayerData->OnInvenSlotChanged.Broadcast(UpdatedSlot, false);
            }
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

    AP1Player* Player = FindEntityAs<AP1Player>(UseItemPkt.entity_id());
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

void UP1GameInstance::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    AP1Player* Player = FindEntityAs<AP1Player>(EquipGearPkt.entity_id());
    if (Player == nullptr)
        return;

    // 공통: 장착 부위 매쉬 변경
    if(EquipGearPkt.success() == true){
        int32 SlotId = EquipGearPkt.slot_id();
        int32 TemplateId = EquipGearPkt.template_id();

        // 서버가 처리 결과로 보낸 장비 부위와 그 부위의 아이템
        Player->ApplyGear(SlotId, TemplateId);
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

void UP1GameInstance::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    AP1Player* Player = FindEntityAs<AP1Player>(UnequipGearPkt.entity_id());
    if (Player == nullptr)
        return;

    // 공통: 장착 부위 매쉬 변경
    if (UnequipGearPkt.success() == true)
    {
        int32 SlotId = UnequipGearPkt.slot_id();
        int32 TemplateId = UnequipGearPkt.template_id();

        // 서버가 처리 결과로 보낸 장비 부위와 그 부위의 아이템
        Player->ApplyGear(SlotId, TemplateId);
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

void UP1GameInstance::HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    AP1Creature* Creature = FindEntityAs<AP1Creature>(NormalAttackPkt.entity_id());
    if (Creature == nullptr)
        return;

    uint32 Combo = NormalAttackPkt.combo();
    float Yaw = NormalAttackPkt.yaw();

    Creature->S_NormalAttack(Combo, Yaw);

}

void UP1GameInstance::HandleHit(const Protocol::S_HIT& HitPkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    AP1Creature* Creature = FindEntityAs<AP1Creature>(HitPkt.entity_id());
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

void UP1GameInstance::HandleDie(const Protocol::S_DIE& DiePkt)
{
    if (Socket == nullptr || GameServerSession == nullptr)
        return;

    AP1Creature* Creature = FindEntityAs<AP1Creature>(DiePkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Die();
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
        UE_LOG(LogP1Network, Warning, TEXT("서버에서 리스폰 실패: %hs"), RespawnPkt.error_message().c_str());
        return;
    }

    // 서버는 같은 액터가 살아나는 것으로 다룬다. 새로 스폰하지 않는다.
    AP1Creature* Creature = FindEntityAs<AP1Creature>(RespawnPkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Respawn(RespawnPkt.pos_info());

    if (Creature->IsMyPlayer())
    {
        for (const Protocol::Stat& Stat_ : RespawnPkt.updated_stat())
        {
            _MyPlayerData->SetStatValue(Stat_.type(), Stat_.value());
            _MyPlayerData->OnStatChangedMappings[Stat_.type()].Broadcast(Stat_.value());
        }
    }
}

UP1MyPlayerData* UP1GameInstance::GetMyPlayerData()
{
    if (IsValid(_MyPlayerData) == false)
        _MyPlayerData = GetSubsystem<UP1MyPlayerData>();

    return _MyPlayerData;
}
