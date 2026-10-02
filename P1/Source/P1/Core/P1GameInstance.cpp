#include "Core/P1GameInstance.h"

#include "Network/P1ConnectionSubsystem.h"
#include "Protocol.pb.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Game/Entities/P1Creature.h"
#include "Game/Progress/P1MyPlayerData.h"
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
    {
        UE_LOG(LogP1System, Warning, TEXT("_MyPlayerData Is Invalid"));
    }
    else
    {
        _MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1GameInstance::HandleMyPlayerSpawned);
    }

    _Connection = GetSubsystem<UP1ConnectionSubsystem>();
    if (IsValid(_Connection) == false)
    {
        UE_LOG(LogP1System, Warning, TEXT("연결 서브시스템을 찾지 못했다"));
    }
    else
    {
        _Connection->OnConnectionLost.AddUObject(this, &UP1GameInstance::HandleConnectionLost);
    }
}

void UP1GameInstance::BeginDestroy()
{
    Super::BeginDestroy();
}

//~ Network Method
#pragma region Network Method

bool UP1GameInstance::IsConnected() const
{
	return _Connection && _Connection->IsConnected();
}

void UP1GameInstance::RemovePacketListener(const UObject* Listener)
{
    OnRecvBuyItemPkt.RemoveAll(Listener);
    OnRecvSellItemPkt.RemoveAll(Listener);
    OnRecvUseItemPkt.RemoveAll(Listener);
    OnRecvEquipGearPkt.RemoveAll(Listener);
    OnRecvUnequipGearPkt.RemoveAll(Listener);
}

void UP1GameInstance::HandleConnectionLost()
{
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

	// 토큰은 게임 서버가 한 번 쓰고 지웠다. 다시 들어가려면 인증 서버에 다시 로그인해야 한다.
	// 그래서 연결만 닫고 다시 잇지 않는다.
	if (_Connection)
		_Connection->Close();

	_MyPlayer = nullptr;

	PendingLoginNotice = Notice;
	UGameplayStatics::OpenLevel(GetWorld(), FName("L_LoginMap"));
}

#pragma endregion Network Method

//~ Handle Packet Method

void UP1GameInstance::HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    if (EnterGamePkt.success() == false)
        return;

    UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>();

    // 게임 서버에 입장한 시점에 가져온 캐릭터의 모든 정보를 저장한다.
    MyPlayerData->InitMyPlayerData(EnterGamePkt);

    // 임시
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

    // 임시
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
            HandleDespawnAll();
            if (EnterRoomPkt.has_enter_pos() && IsValid(_MyPlayer))
            {
                _MyPlayer->SetClientPos(EnterRoomPkt.enter_pos());
                _MyPlayer->SetServerPos(EnterRoomPkt.enter_pos());
            }
        }
        
    }

}

void UP1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
    if (IsConnected() == false)
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
    if (IsConnected() == false)
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

void UP1GameInstance::HandleDespawnAll()
{
    if (IsConnected() == false)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    if (UP1StatefulEntityManager* StatefulEntityManager = World->GetSubsystem<UP1StatefulEntityManager>())
    {
        StatefulEntityManager->DespawnAllEntities();
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
	if (IsConnected() == false)
		return;

	//auto* World = GetWorld();
	//if (World == nullptr)
	//	return;

    for(auto& info : MovePkt.info())
        HandleMove(info);
}

void UP1GameInstance::HandleBuyItem(const Protocol::S_BUY_ITEM& BuyItemPkt)
{
    if (IsConnected() == false)
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
    if (IsConnected() == false)
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
    if (IsConnected() == false)
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

            _MyPlayerData->ApplyStats(UseItemPkt.updated_stat());
        }
    }
}

void UP1GameInstance::HandleEquipGear(const Protocol::S_EQUIP_GEAR& EquipGearPkt)
{
    if (IsConnected() == false)
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

            _MyPlayerData->ApplyStats(EquipGearPkt.updated_stat());

        }

    }

}

void UP1GameInstance::HandleUnequipGear(const Protocol::S_UNEQUIP_GEAR& UnequipGearPkt)
{
    if (IsConnected() == false)
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

            _MyPlayerData->ApplyStats(UnequipGearPkt.updated_stat());

        }

    }


}

void UP1GameInstance::HandleNormalAttack(const Protocol::S_NORMAL_ATTACK& NormalAttackPkt)
{
    if (IsConnected() == false)
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
    if (IsConnected() == false)
        return;

    AP1Creature* Creature = FindEntityAs<AP1Creature>(HitPkt.entity_id());
    if (Creature == nullptr)
        return;

    // 피격 연출과 HP 갱신
    Creature->S_Hit(HitPkt.damage(), HitPkt.updated_hp());

    if (Creature->IsMyPlayer())
    {
        _MyPlayerData->ApplyStat(Protocol::STAT_TYPE_HP, HitPkt.updated_hp());
    }
}

void UP1GameInstance::HandleDie(const Protocol::S_DIE& DiePkt)
{
    if (IsConnected() == false)
        return;

    AP1Creature* Creature = FindEntityAs<AP1Creature>(DiePkt.entity_id());
    if (Creature == nullptr)
        return;

    Creature->S_Die();
}

void UP1GameInstance::HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt)
{
    if (IsConnected() == false)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    

}

void UP1GameInstance::HandleRespawn(const Protocol::S_RESPAWN& RespawnPkt)
{
    if (IsConnected() == false)
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
        _MyPlayerData->ApplyStats(RespawnPkt.updated_stat());
    }
}

void UP1GameInstance::HandleMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    _MyPlayer = MyPlayer;
}

UP1MyPlayerData* UP1GameInstance::GetMyPlayerData()
{
    if (IsValid(_MyPlayerData) == false)
        _MyPlayerData = GetSubsystem<UP1MyPlayerData>();

    return _MyPlayerData;
}
