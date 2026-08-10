#include "pch.h"
#include "ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	//if (pkt.success() == false)
	//	return true;

	//if (pkt.players().size() == 0)
	//{
	//	// 캐릭터 생성창
	//}

	//// 입장 UI 버튼 눌러서 게임 입장
	//Protocol::C_ENTER_GAME enterGamePkt;
	//enterGamePkt.set_playerindex(0); // 첫번째 캐릭터로 입장
	//auto sendBuffer = ClientPacketHandler::MakeSerializedPacket(enterGamePkt);
	//session->Send(sendBuffer);

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

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	return false;
}

bool Handle_S_ENTER_MAP(PacketSessionRef& session, Protocol::S_ENTER_MAP& pkt)
{
	return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
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

bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt)
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

bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt)
{
	return false;
}

bool Handle_S_REWARD_RESULT(PacketSessionRef& session, Protocol::S_REWARD_RESULT& pkt)
{
	return false;
}

bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt)
{
	return false;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	std::cout << pkt.msg() << endl;

	return true;
}

