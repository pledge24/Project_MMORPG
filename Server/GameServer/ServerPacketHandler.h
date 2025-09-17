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
	PKT_C_ENTER_MAP_COMPLETE = 1010,
	PKT_C_MOVE_ROOM = 1011,
	PKT_S_MOVE_ROOM = 1012,
	PKT_C_LEAVE_GAME = 1013,
	PKT_S_LEAVE_GAME = 1014,
	PKT_S_SPAWN = 1015,
	PKT_S_DESPAWN = 1016,
	PKT_C_MOVE = 1017,
	PKT_S_MOVE = 1018,
	PKT_C_ATTACK = 1019,
	PKT_S_ATTACK = 1020,
	PKT_S_HIT = 1021,
	PKT_C_BUY_ITEM = 1022,
	PKT_S_BUY_ITEM = 1023,
	PKT_C_SELL_ITEM = 1024,
	PKT_S_SELL_ITEM = 1025,
	PKT_C_EQUIP_GEAR = 1026,
	PKT_S_EQUIP_GEAR = 1027,
	PKT_C_UNEQUIP_GEAR = 1028,
	PKT_S_UNEQUIP_GEAR = 1029,
	PKT_C_USE_ITEM = 1030,
	PKT_S_USE_ITEM = 1031,
};

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

// Auto-generated template Handle Functions
bool Handle_C_PING(PacketSessionRef& session, Protocol::C_PING& pkt);
bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt);
bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt);
bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt);
bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt);
bool Handle_C_ENTER_MAP_COMPLETE(PacketSessionRef& session, Protocol::C_ENTER_MAP_COMPLETE& pkt);
bool Handle_C_MOVE_ROOM(PacketSessionRef& session, Protocol::C_MOVE_ROOM& pkt);
bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt);
bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt);
bool Handle_C_ATTACK(PacketSessionRef& session, Protocol::C_ATTACK& pkt);
bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt);
bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt);
bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt);
bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt);
bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		// Auto-generated
		GPacketHandler[PKT_C_PING] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_PING>(Handle_C_PING, session, buffer, len); };
		GPacketHandler[PKT_C_LOGIN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_C_CREATE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_CREATE_CHARACTER>(Handle_C_CREATE_CHARACTER, session, buffer, len); };
		GPacketHandler[PKT_C_DELETE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_DELETE_CHARACTER>(Handle_C_DELETE_CHARACTER, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_GAME>(Handle_C_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_MAP_COMPLETE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ENTER_MAP_COMPLETE>(Handle_C_ENTER_MAP_COMPLETE, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE_ROOM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE_ROOM>(Handle_C_MOVE_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_LEAVE_GAME>(Handle_C_LEAVE_GAME, session, buffer, len); };
		GPacketHandler[PKT_C_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_MOVE>(Handle_C_MOVE, session, buffer, len); };
		GPacketHandler[PKT_C_ATTACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_ATTACK>(Handle_C_ATTACK, session, buffer, len); };
		GPacketHandler[PKT_C_BUY_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_BUY_ITEM>(Handle_C_BUY_ITEM, session, buffer, len); };
		GPacketHandler[PKT_C_SELL_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_SELL_ITEM>(Handle_C_SELL_ITEM, session, buffer, len); };
		GPacketHandler[PKT_C_EQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_EQUIP_GEAR>(Handle_C_EQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_C_UNEQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_UNEQUIP_GEAR>(Handle_C_UNEQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_C_USE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::C_USE_ITEM>(Handle_C_USE_ITEM, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}

	// Auto-generated
	static SendBufferRef MakeSerializedPacket(Protocol::S_PONG& pkt) { return MakeSerializedPacket(pkt, PKT_S_PONG); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_LOGIN& pkt) { return MakeSerializedPacket(pkt, PKT_S_LOGIN); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_CREATE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_S_CREATE_CHARACTER); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_DELETE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_S_DELETE_CHARACTER); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_ENTER_GAME& pkt) { return MakeSerializedPacket(pkt, PKT_S_ENTER_GAME); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_MOVE_ROOM& pkt) { return MakeSerializedPacket(pkt, PKT_S_MOVE_ROOM); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_LEAVE_GAME& pkt) { return MakeSerializedPacket(pkt, PKT_S_LEAVE_GAME); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_SPAWN& pkt) { return MakeSerializedPacket(pkt, PKT_S_SPAWN); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_DESPAWN& pkt) { return MakeSerializedPacket(pkt, PKT_S_DESPAWN); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_MOVE& pkt) { return MakeSerializedPacket(pkt, PKT_S_MOVE); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_ATTACK& pkt) { return MakeSerializedPacket(pkt, PKT_S_ATTACK); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_HIT& pkt) { return MakeSerializedPacket(pkt, PKT_S_HIT); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_BUY_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_S_BUY_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_SELL_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_S_SELL_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_EQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_S_EQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_UNEQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_S_UNEQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::S_USE_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_S_USE_ITEM); }

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