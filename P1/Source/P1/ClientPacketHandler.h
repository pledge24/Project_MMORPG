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
	PKT_S_SPAWN = 1012,
	PKT_S_DESPAWN = 1013,
	PKT_C_MOVE = 1014,
	PKT_S_MOVE = 1015,
	PKT_C_CHAT = 1016,
	PKT_S_CHAT = 1017,
};

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

// Auto-generated template Handle Functions
bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt);
bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt);
bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt);
bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt);
bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt);
bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt);
bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt);
bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt);
bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt);
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
		GPacketHandler[PKT_S_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE>(Handle_S_MOVE, session, buffer, len); };
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
	static SendBufferRef MakeSerializedPacket(Protocol::C_MOVE& pkt) { return MakeSerializedPacket(pkt, PKT_C_MOVE); }
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