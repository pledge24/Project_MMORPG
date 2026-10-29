#include "Main/pch.h"
#include "Main/ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_DISCONNECT(PacketSessionRef& session, Protocol::S_DISCONNECT& pkt)
{
	return false;
}

bool Handle_S_CONNECT_LOBBY(PacketSessionRef& session, Protocol::S_CONNECT_LOBBY& pkt)
{
	return true;
}

bool Handle_S_CHARACTER_LIST(PacketSessionRef& session, Protocol::S_CHARACTER_LIST& pkt)
{
	return true;
}

bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt)
{
	return false;
}

bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt)
{
    return false;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
{
	return false;
}

bool Handle_S_USE_PORTAL(PacketSessionRef& session, Protocol::S_USE_PORTAL& pkt)
{
	return false;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	return false;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
    return false;
}

bool Handle_S_GEAR_APPEARANCE(PacketSessionRef& session, Protocol::S_GEAR_APPEARANCE& pkt)
{
	return false;
}

bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt)
{
	return false;
}

bool Handle_S_ATTACK(PacketSessionRef& session, Protocol::S_ATTACK& pkt)
{
	return false;
}

bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt)
{
	return false;
}

bool Handle_S_BUY_ITEM(PacketSessionRef& session, Protocol::S_BUY_ITEM& pkt)
{
	return false;
}

bool Handle_S_SELL_ITEM(PacketSessionRef& session, Protocol::S_SELL_ITEM& pkt)
{
	return false;
}

bool Handle_S_EQUIP_GEAR(PacketSessionRef& session, Protocol::S_EQUIP_GEAR& pkt)
{
	return false;
}

bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt)
{
	return false;
}

bool Handle_S_USE_ITEM(PacketSessionRef& session, Protocol::S_USE_ITEM& pkt)
{
	return false;
}

bool Handle_S_SET_QUICK_SLOT(PacketSessionRef& session, Protocol::S_SET_QUICK_SLOT& pkt)
{
	return false;
}

bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt)
{
	return false;
}

bool Handle_S_REWARD(PacketSessionRef& session, Protocol::S_REWARD& pkt)
{
	return false;
}

bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt)
{
	return false;
}

bool Handle_S_RETURN_TO_LOBBY(PacketSessionRef& session, Protocol::S_RETURN_TO_LOBBY& pkt)
{
	return false;
}

bool Handle_S_CHANNEL_LIST(PacketSessionRef& session, Protocol::S_CHANNEL_LIST& pkt)
{
	return false;
}

bool Handle_S_CHANGE_CHANNEL(PacketSessionRef& session, Protocol::S_CHANGE_CHANNEL& pkt)
{
	return false;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	return true;
}

bool Handle_S_CHAT_MESSAGE(PacketSessionRef& session, Protocol::S_CHAT_MESSAGE& pkt)
{
	GLogger->Info("채팅: {}", pkt.message());

	return true;
}

