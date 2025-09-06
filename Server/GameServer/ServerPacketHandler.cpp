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
                DBRequestFunctions::GetUserCharactersData(session, userId);
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
        [session, pkt, player]()
        {
            int64 characterId = pkt.character_id();
            DBRequestFunctions::GetEnterGameData(session, characterId);
            GRoom->DoAsync(&Room::HandleEnterPlayer, player);
        }
    );

    dbQueue->Push(std::move(job));

	// 방에 입장

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

	GRoom->DoAsync(&Room::HandleLeavePlayer, player);

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

	// TODO

	GRoom->DoAsync(&Room::HandleMove, pkt);

	return true;
}

bool Handle_C_ATTACK(PacketSessionRef& session, Protocol::C_ATTACK& pkt)
{
    return false;
}

bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    Protocol::S_BUY_ITEM rPkt;
    Protocol::Slot* updatedSlot = rPkt.mutable_updated_slot();

    // 1. 충분한 돈이 있는지 Validation
    int64 gold = player->playerInfo->gold();
    int32 templateId = pkt.template_id();
    int64 buyPrice = Gamedata::ItemDataTable[templateId]["buyPrice"];

    if (gold < buyPrice)
        return false;

    // 2. 구매 후 금액 정산
    int64 totalGold = gold - buyPrice;
    player->playerInfo->set_gold(totalGold);

    // 3. 패킷 필드 세팅 및 전송
    rPkt.set_gold(totalGold);
    SEND_PACKET(rPkt);


    return true;
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    Protocol::S_SELL_ITEM rPkt;
    Protocol::Slot* targetSlot = pkt.mutable_slot();
    Protocol::Slot* updatedSlot = rPkt.mutable_updated_slot();

    // 1. "해당 슬롯"의 Validation(아이템 uid 확인, 수량 확인)
    if (player->inventory->removeItem(updatedSlot, targetSlot) == false)
        return false;

    // 2. 판매 후 금액 정산
    int64 gold = player->playerInfo->gold();
    int32 templateId = targetSlot->item().template_id();
    int64 sellPrice = Gamedata::ItemDataTable[templateId]["sellPrice"];
    
    int64 totalGold = gold + sellPrice;
    player->playerInfo->set_gold(totalGold);

    // 3. 패킷 필드 세팅 및 전송
    rPkt.set_gold(totalGold);
    SEND_PACKET(rPkt);
    

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

    GRoom->DoAsync(&Room::HandleEquipGear, pkt, player);

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

    GRoom->DoAsync(&Room::HandleUnequipGear, pkt, player);

    return true;
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    return true;
}

