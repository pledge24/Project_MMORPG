#include "pch.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include "GameSession.h"
#include "Player.h"
#include "Room.h"
#include "ObjectUtils.h"
#include "Inventory.h"
#include "EquippedGear.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

bool Handle_C_PING(PacketSessionRef& session, Protocol::C_PING& pkt)
{
	return false;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 랜덤으로 아무 DBQueue에게 Job을 준다.
    int32 dbQueueCount = GDBManager->GetDBQueueCount();
    DBQueueRef dbQueue = GDBManager->GetDBQueue(Utils::GetRandom(0, dbQueueCount));

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            // 클라로부터 받은 AccessToken을 Redis와 비교
            string accessToken = pkt.access_token();

            RedisRef redis = GRedisManager->GetRedis();
            auto val = redis->get("accessToken:" + accessToken);
            if (val)
            {
                Json json = Json::parse(*val);
                string username = json["username"];
                int64 userId = json["userId"];

                cout << "userId: " << userId << endl;
                cout << "username: " << username << endl;
                
                // 게임 세션에 userId 저장.
                GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
                gameSession->userId = userId;

                // 있으면 DB에서 캐릭터 정보를 긁어온다.
                DBRequestFunctions::LoadUserCharactersData(session, userId);
            }
            else
            {
                cout << "Not Found AccessToken" << endl;
            }
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt, userId]()
        {
            const Protocol::CharacterOverview& character = pkt.character();
            DBRequestFunctions::CreateCharacter(session, character, userId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            int64 characterId = pkt.character_id();
            DBRequestFunctions::DeleteCharacter(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	// 플레이어 생성
	PlayerRef player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            int64 characterId = pkt.character_id();
            DBRequestFunctions::LoadAllCharactersData(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_ENTER_MAP_COMPLETE(PacketSessionRef& session, Protocol::C_ENTER_MAP_COMPLETE& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    int32 roomId = player->objectInfo->map_id();
    RoomRef room = GRoomManager->GetRoomRefFromRoomId(roomId);
    if (room == nullptr)
        return false;

    // Room 입장 처리
    {
        shared_ptr<Protocol::PosInfo> enterPos = make_shared<Protocol::PosInfo>();
        enterPos->CopyFrom(player->objectInfo->pos_info());

        room->DoAsync(&Room::HandleEnterPlayer, player, enterPos, false);
    }

    return true;
}

bool Handle_C_MOVE_ROOM(PacketSessionRef& session, Protocol::C_MOVE_ROOM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef curRoom = player->room.load().lock();
    if (curRoom == nullptr)
        return false;

    optional<Json> opt = curRoom->GetPortalDataFromPortalId(pkt.portal_id());
    if (opt.has_value() == false)
    {
        wcout << "플레이어가 현재 Room에 존재하지 않는 포탈사용 시도" << '\n';
        return false;
    }

    // Room 이동 처리
    {
        using namespace JsonProperty::Map;
        const Json& portalData = opt.value();
        const Json& dst = portalData[Dst];

        // 1) 이동할 위치 설정
        shared_ptr<Protocol::PosInfo> enterPos = make_shared<Protocol::PosInfo>();
        {
            enterPos->set_object_id(player->objectInfo->object_id());
            enterPos->set_x(dst[PosX]);
            enterPos->set_y(dst[PosY]);
            enterPos->set_z(dst[PosZ]);
            enterPos->set_yaw(dst[Yaw]);
            enterPos->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        }

        // 2) Room 현재 room은 나가고, 다음 room은 들어간다.
        {
            RoomRef enterRoom = GRoomManager->GetRoomRefFromRoomId(dst[TemplateId]);

            curRoom->DoAsync(&Room::HandleLeavePlayer, player, true);
            enterRoom->DoAsync(&Room::HandleEnterPlayer, player, enterPos, true);
        }
    }

    return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.load().lock();
	if (room == nullptr)
		return false;

    // Room 퇴장 처리
    {
        room->DoAsync(&Room::HandleLeavePlayer, player, false);
	}

    // DB 업데이트 처리
	{
        int64 characterId = player->playerInfo->character_id();
        DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(characterId);

        // 게임 종료 플레이어 정보 DB에 저장.
        JobRef job = make_shared<Job>(
            [session, player]()
            {
                DBRequestFunctions::UpdateAllCharactersData(session);
            }
        );
        dbQueue->Push(std::move(job));
	}

    // GameSession 네트워크 연결 해제
    gameSession->Disconnect("Exit Game");

	return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.load().lock();
	if (room == nullptr)
		return false;

    room->DoAsync(&Room::HandleMove, pkt);

	return true;
}

bool Handle_C_NORMAL_ATTACK(PacketSessionRef& session, Protocol::C_NORMAL_ATTACK& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::HandleNormalAttack, pkt, player);

    return true;
}



bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    Protocol::S_BUY_ITEM buyItemPkt;
    Protocol::Slot* updatedSlot = buyItemPkt.mutable_updated_slot();
    int32 templateId = pkt.template_id();
    int64 totalGold = 0;

    if (player->HandleBuyItem(OUT updatedSlot, OUT totalGold, templateId) == false)
    {
        buyItemPkt.set_success(false);
        SEND_PACKET(buyItemPkt);
        return false;
    }

    buyItemPkt.set_success(true);
    buyItemPkt.set_gold(totalGold);
    SEND_PACKET(buyItemPkt);
    cout << buyItemPkt.DebugString() << endl;

    return true;
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    Protocol::S_SELL_ITEM sellItemPkt;
    Protocol::Slot* targetSlot = pkt.mutable_slot();
    Protocol::Slot* updatedSlot = sellItemPkt.mutable_updated_slot();

    int64 totalGold = 0;
    if (player->HandleSellItem(OUT updatedSlot, targetSlot, OUT totalGold) == false)
    {
        sellItemPkt.set_success(false);
        SEND_PACKET(sellItemPkt);
        return false;
    }

    sellItemPkt.set_success(true);
    sellItemPkt.set_gold(totalGold);
    SEND_PACKET(sellItemPkt);
    
    return true;
}

bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::HandleEquipGear, pkt, player);

    return true;
}


bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::HandleUnequipGear, pkt, player);

    return true;
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    Protocol::S_USE_ITEM useItemPkt;
    Protocol::Slot* targetSlot = pkt.mutable_slot();
    if (player->HandleUseItem(useItemPkt, targetSlot) == false)
    {
        useItemPkt.set_success(false);
        SEND_PACKET(useItemPkt);
        return false;
    }

    useItemPkt.set_success(true);
    SEND_PACKET(useItemPkt);

    return true;
}

