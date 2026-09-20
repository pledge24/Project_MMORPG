#pragma once
#include "Protocol.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "SendBuffer.h"
#include "Types.h"
#endif

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Auto-generated
enum : uint16
{
	PKT_C_PING = 1000,
	PKT_S_PONG = 1001,
	PKT_C_LOGIN = 1002,
	PKT_S_LOGIN = 1003,
	PKT_C_CREATE_CHARACTER = 1004,
	PKT_S_CREATE_CHARACTER = 1005,
	PKT_C_DELETE_CHARACTER = 1006,
	PKT_S_DELETE_CHARACTER = 1007,
	PKT_C_ENTER_GAME = 1008,
	PKT_S_ENTER_GAME = 1009,
	PKT_C_LEAVE_GAME = 1010,
	PKT_S_LEAVE_GAME = 1011,
	PKT_C_MAP_LOAD_COMPLETE = 1012,
	PKT_C_ENTER_MAP = 1013,
	PKT_S_ENTER_MAP = 1014,
	PKT_C_ENTER_ROOM = 1015,
	PKT_S_ENTER_ROOM = 1016,
	PKT_S_SPAWN = 1017,
	PKT_S_DESPAWN = 1018,
	PKT_C_MOVE = 1019,
	PKT_S_MOVE = 1020,
	PKT_C_NORMAL_ATTACK = 1021,
	PKT_S_NORMAL_ATTACK = 1022,
	PKT_S_HIT = 1023,
	PKT_C_BUY_ITEM = 1024,
	PKT_S_BUY_ITEM = 1025,
	PKT_C_SELL_ITEM = 1026,
	PKT_S_SELL_ITEM = 1027,
	PKT_C_EQUIP_GEAR = 1028,
	PKT_S_EQUIP_GEAR = 1029,
	PKT_C_UNEQUIP_GEAR = 1030,
	PKT_S_UNEQUIP_GEAR = 1031,
	PKT_C_USE_ITEM = 1032,
	PKT_S_USE_ITEM = 1033,
	PKT_S_DIE = 1034,
	PKT_S_REWARD_RESULT = 1035,
	PKT_C_RESPAWN = 1036,
	PKT_S_RESPAWN = 1037,
	PKT_C_CHAT = 1038,
	PKT_S_CHAT = 1039,
};

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

// Auto-generated template Handle Functions
bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt);
bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt);
bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt);
bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt);
bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt);
bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt);
bool Handle_S_ENTER_MAP(PacketSessionRef& session, Protocol::S_ENTER_MAP& pkt);
bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt);
bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt);
bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt);
bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt);
bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt);
bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt);
bool Handle_S_BUY_ITEM(PacketSessionRef& session, Protocol::S_BUY_ITEM& pkt);
bool Handle_S_SELL_ITEM(PacketSessionRef& session, Protocol::S_SELL_ITEM& pkt);
bool Handle_S_EQUIP_GEAR(PacketSessionRef& session, Protocol::S_EQUIP_GEAR& pkt);
bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt);
bool Handle_S_USE_ITEM(PacketSessionRef& session, Protocol::S_USE_ITEM& pkt);
bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt);
bool Handle_S_REWARD_RESULT(PacketSessionRef& session, Protocol::S_REWARD_RESULT& pkt);
bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt);
bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		// Auto-generated
		GPacketHandler[PKT_S_PONG] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PONG>(Handle_S_PONG, session, buffer, len); };
		GPacketHandler[PKT_S_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN>(Handle_S_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S_CREATE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CREATE_CHARACTER>(Handle_S_CREATE_CHARACTER, session, buffer, len); };
		GPacketHandler[PKT_S_DELETE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DELETE_CHARACTER>(Handle_S_DELETE_CHARACTER, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_GAME>(Handle_S_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_S_LEAVE_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LEAVE_GAME>(Handle_S_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_MAP] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_MAP>(Handle_S_ENTER_MAP, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_ROOM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_ROOM>(Handle_S_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE>(Handle_S_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_NORMAL_ATTACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_NORMAL_ATTACK>(Handle_S_NORMAL_ATTACK, session, buffer, len); };
		GPacketHandler[PKT_S_HIT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_HIT>(Handle_S_HIT, session, buffer, len); };
		GPacketHandler[PKT_S_BUY_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_BUY_ITEM>(Handle_S_BUY_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_SELL_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SELL_ITEM>(Handle_S_SELL_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_EQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_EQUIP_GEAR>(Handle_S_EQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_S_UNEQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_UNEQUIP_GEAR>(Handle_S_UNEQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_S_USE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_USE_ITEM>(Handle_S_USE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_DIE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DIE>(Handle_S_DIE, session, buffer, len); };
		GPacketHandler[PKT_S_REWARD_RESULT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_REWARD_RESULT>(Handle_S_REWARD_RESULT, session, buffer, len); };
		GPacketHandler[PKT_S_RESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_RESPAWN>(Handle_S_RESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT>(Handle_S_CHAT, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}

	// Auto-generated
	static SendBufferRef MakeSerializedPacket(Protocol::C_PING& pkt) { return MakeSerializedPacket(pkt, PKT_C_PING); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_LOGIN& pkt) { return MakeSerializedPacket(pkt, PKT_C_LOGIN); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CREATE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_C_CREATE_CHARACTER); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_DELETE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_C_DELETE_CHARACTER); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_ENTER_GAME& pkt) { return MakeSerializedPacket(pkt, PKT_C_ENTER_GAME); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_LEAVE_GAME& pkt) { return MakeSerializedPacket(pkt, PKT_C_LEAVE_GAME); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_MAP_LOAD_COMPLETE& pkt) { return MakeSerializedPacket(pkt, PKT_C_MAP_LOAD_COMPLETE); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_ENTER_MAP& pkt) { return MakeSerializedPacket(pkt, PKT_C_ENTER_MAP); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_ENTER_ROOM& pkt) { return MakeSerializedPacket(pkt, PKT_C_ENTER_ROOM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_MOVE& pkt) { return MakeSerializedPacket(pkt, PKT_C_MOVE); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_NORMAL_ATTACK& pkt) { return MakeSerializedPacket(pkt, PKT_C_NORMAL_ATTACK); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_BUY_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_BUY_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_SELL_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_SELL_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_EQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_C_EQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_UNEQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_C_UNEQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_USE_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_USE_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_RESPAWN& pkt) { return MakeSerializedPacket(pkt, PKT_C_RESPAWN); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CHAT& pkt) { return MakeSerializedPacket(pkt, PKT_C_CHAT); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSerializedPacket(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
		SendBufferRef sendBuffer = MakeShared<SendBuffer>(packetSize);
#else
		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
#endif

		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		pkt.SerializeToArray(&header[1], dataSize);
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};