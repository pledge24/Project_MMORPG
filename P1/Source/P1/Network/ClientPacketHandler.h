#pragma once
#include "Chat.pb.h"
#include "Combat.pb.h"
#include "Common.pb.h"
#include "Entity.pb.h"
#include "Entry.pb.h"
#include "Item.pb.h"
#include "Lobby.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "Network/SendBuffer.h"
#include "Utils/Types.h"
#include "Utils/LogCategory.h"
#endif

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// 패킷 번호. 스키마 파일마다 정한 범위 안에서 선언 순서로 매긴다.
enum : uint16
{
	PKT_C_CHAT = 700,
	PKT_S_CHAT = 701,
	PKT_S_CHAT_MESSAGE = 702,
	PKT_C_NORMAL_ATTACK = 500,
	PKT_S_NORMAL_ATTACK = 501,
	PKT_S_ATTACK = 502,
	PKT_S_HIT = 503,
	PKT_S_DIE = 504,
	PKT_S_REWARD = 505,
	PKT_S_DISCONNECT = 100,
	PKT_S_SPAWN = 400,
	PKT_S_DESPAWN = 401,
	PKT_C_MOVE = 402,
	PKT_S_MOVE = 403,
	PKT_S_GEAR_APPEARANCE = 404,
	PKT_C_ENTER_GAME = 300,
	PKT_S_ENTER_GAME = 301,
	PKT_C_MAP_LOAD_COMPLETE = 302,
	PKT_S_ENTER_ROOM = 303,
	PKT_C_USE_PORTAL = 304,
	PKT_S_USE_PORTAL = 305,
	PKT_C_RESPAWN = 306,
	PKT_S_RESPAWN = 307,
	PKT_C_RETURN_TO_LOBBY = 308,
	PKT_S_RETURN_TO_LOBBY = 309,
	PKT_C_CHANNEL_LIST = 310,
	PKT_S_CHANNEL_LIST = 311,
	PKT_C_CHANGE_CHANNEL = 312,
	PKT_S_CHANGE_CHANNEL = 313,
	PKT_C_BUY_ITEM = 600,
	PKT_S_BUY_ITEM = 601,
	PKT_C_SELL_ITEM = 602,
	PKT_S_SELL_ITEM = 603,
	PKT_C_EQUIP_GEAR = 604,
	PKT_S_EQUIP_GEAR = 605,
	PKT_C_UNEQUIP_GEAR = 606,
	PKT_S_UNEQUIP_GEAR = 607,
	PKT_C_USE_ITEM = 608,
	PKT_S_USE_ITEM = 609,
	PKT_C_SET_QUICK_SLOT = 610,
	PKT_S_SET_QUICK_SLOT = 611,
	PKT_C_USE_QUICK_SLOT = 612,
	PKT_C_CONNECT_LOBBY = 200,
	PKT_S_CONNECT_LOBBY = 201,
	PKT_S_CHARACTER_LIST = 202,
	PKT_C_CREATE_CHARACTER = 203,
	PKT_S_CREATE_CHARACTER = 204,
	PKT_C_DELETE_CHARACTER = 205,
	PKT_S_DELETE_CHARACTER = 206,
};

/** 생성기가 읽은 스키마 파일과 그 파일의 패킷 번호 범위. 프로토콜 계약 테스트가 자기 표와 대조한다. */
struct PacketSchemaFile
{
	const char* name;
	uint16 firstPacketId;	// 범위가 없는 파일은 0
	uint16 lastPacketId;	// 범위가 없는 파일은 0
};

inline constexpr PacketSchemaFile PACKET_SCHEMA_FILES[] =
{
	{ "Chat.proto", 700, 799 },
	{ "Combat.proto", 500, 599 },
	{ "Common.proto", 100, 199 },
	{ "Entity.proto", 400, 499 },
	{ "Entry.proto", 300, 399 },
	{ "Item.proto", 600, 699 },
	{ "Lobby.proto", 200, 299 },
};

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

// 받는 패킷의 핸들러. 정의는 손으로 쓴다.
bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt);
bool Handle_S_CHAT_MESSAGE(PacketSessionRef& session, Protocol::S_CHAT_MESSAGE& pkt);
bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt);
bool Handle_S_ATTACK(PacketSessionRef& session, Protocol::S_ATTACK& pkt);
bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt);
bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt);
bool Handle_S_REWARD(PacketSessionRef& session, Protocol::S_REWARD& pkt);
bool Handle_S_DISCONNECT(PacketSessionRef& session, Protocol::S_DISCONNECT& pkt);
bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt);
bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt);
bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt);
bool Handle_S_GEAR_APPEARANCE(PacketSessionRef& session, Protocol::S_GEAR_APPEARANCE& pkt);
bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt);
bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt);
bool Handle_S_USE_PORTAL(PacketSessionRef& session, Protocol::S_USE_PORTAL& pkt);
bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt);
bool Handle_S_RETURN_TO_LOBBY(PacketSessionRef& session, Protocol::S_RETURN_TO_LOBBY& pkt);
bool Handle_S_CHANNEL_LIST(PacketSessionRef& session, Protocol::S_CHANNEL_LIST& pkt);
bool Handle_S_CHANGE_CHANNEL(PacketSessionRef& session, Protocol::S_CHANGE_CHANNEL& pkt);
bool Handle_S_BUY_ITEM(PacketSessionRef& session, Protocol::S_BUY_ITEM& pkt);
bool Handle_S_SELL_ITEM(PacketSessionRef& session, Protocol::S_SELL_ITEM& pkt);
bool Handle_S_EQUIP_GEAR(PacketSessionRef& session, Protocol::S_EQUIP_GEAR& pkt);
bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt);
bool Handle_S_USE_ITEM(PacketSessionRef& session, Protocol::S_USE_ITEM& pkt);
bool Handle_S_SET_QUICK_SLOT(PacketSessionRef& session, Protocol::S_SET_QUICK_SLOT& pkt);
bool Handle_S_CONNECT_LOBBY(PacketSessionRef& session, Protocol::S_CONNECT_LOBBY& pkt);
bool Handle_S_CHARACTER_LIST(PacketSessionRef& session, Protocol::S_CHARACTER_LIST& pkt);
bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt);
bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		// 받는 패킷마다 핸들러를 등록한다.
		GPacketHandler[PKT_S_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT>(Handle_S_CHAT, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT_MESSAGE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT_MESSAGE>(Handle_S_CHAT_MESSAGE, session, buffer, len); };
		GPacketHandler[PKT_S_NORMAL_ATTACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_NORMAL_ATTACK>(Handle_S_NORMAL_ATTACK, session, buffer, len); };
		GPacketHandler[PKT_S_ATTACK] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ATTACK>(Handle_S_ATTACK, session, buffer, len); };
		GPacketHandler[PKT_S_HIT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_HIT>(Handle_S_HIT, session, buffer, len); };
		GPacketHandler[PKT_S_DIE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DIE>(Handle_S_DIE, session, buffer, len); };
		GPacketHandler[PKT_S_REWARD] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_REWARD>(Handle_S_REWARD, session, buffer, len); };
		GPacketHandler[PKT_S_DISCONNECT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DISCONNECT>(Handle_S_DISCONNECT, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE>(Handle_S_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_GEAR_APPEARANCE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_GEAR_APPEARANCE>(Handle_S_GEAR_APPEARANCE, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_GAME] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_GAME>(Handle_S_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_ROOM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_ROOM>(Handle_S_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_S_USE_PORTAL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_USE_PORTAL>(Handle_S_USE_PORTAL, session, buffer, len); };
		GPacketHandler[PKT_S_RESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_RESPAWN>(Handle_S_RESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_RETURN_TO_LOBBY] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_RETURN_TO_LOBBY>(Handle_S_RETURN_TO_LOBBY, session, buffer, len); };
		GPacketHandler[PKT_S_CHANNEL_LIST] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANNEL_LIST>(Handle_S_CHANNEL_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_CHANGE_CHANNEL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHANGE_CHANNEL>(Handle_S_CHANGE_CHANNEL, session, buffer, len); };
		GPacketHandler[PKT_S_BUY_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_BUY_ITEM>(Handle_S_BUY_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_SELL_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SELL_ITEM>(Handle_S_SELL_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_EQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_EQUIP_GEAR>(Handle_S_EQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_S_UNEQUIP_GEAR] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_UNEQUIP_GEAR>(Handle_S_UNEQUIP_GEAR, session, buffer, len); };
		GPacketHandler[PKT_S_USE_ITEM] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_USE_ITEM>(Handle_S_USE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_SET_QUICK_SLOT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SET_QUICK_SLOT>(Handle_S_SET_QUICK_SLOT, session, buffer, len); };
		GPacketHandler[PKT_S_CONNECT_LOBBY] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CONNECT_LOBBY>(Handle_S_CONNECT_LOBBY, session, buffer, len); };
		GPacketHandler[PKT_S_CHARACTER_LIST] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHARACTER_LIST>(Handle_S_CHARACTER_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_CREATE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CREATE_CHARACTER>(Handle_S_CREATE_CHARACTER, session, buffer, len); };
		GPacketHandler[PKT_S_DELETE_CHARACTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DELETE_CHARACTER>(Handle_S_DELETE_CHARACTER, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		// 헤더보다 짧으면 헤더를 읽을 수 없다. 읽으면 받은 길이 밖의 바이트를 id로 쓰고, 본문 길이가 음수가 된다.
		if (len < static_cast<int32>(sizeof(PacketHeader)))
			return Handle_INVALID(session, buffer, len);

		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// 테이블은 UINT16_MAX 칸이라 id 65535는 범위 밖이다. id는 상대가 보낸 값이므로 믿지 않는다.
		if (header->id >= UINT16_MAX)
			return Handle_INVALID(session, buffer, len);

		return GPacketHandler[header->id](session, buffer, len);
	}

	// 보내는 패킷의 직렬화.
	static SendBufferRef MakeSerializedPacket(Protocol::C_CHAT& pkt) { return MakeSerializedPacket(pkt, PKT_C_CHAT); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_NORMAL_ATTACK& pkt) { return MakeSerializedPacket(pkt, PKT_C_NORMAL_ATTACK); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_MOVE& pkt) { return MakeSerializedPacket(pkt, PKT_C_MOVE); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_ENTER_GAME& pkt) { return MakeSerializedPacket(pkt, PKT_C_ENTER_GAME); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_MAP_LOAD_COMPLETE& pkt) { return MakeSerializedPacket(pkt, PKT_C_MAP_LOAD_COMPLETE); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_USE_PORTAL& pkt) { return MakeSerializedPacket(pkt, PKT_C_USE_PORTAL); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_RESPAWN& pkt) { return MakeSerializedPacket(pkt, PKT_C_RESPAWN); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_RETURN_TO_LOBBY& pkt) { return MakeSerializedPacket(pkt, PKT_C_RETURN_TO_LOBBY); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CHANNEL_LIST& pkt) { return MakeSerializedPacket(pkt, PKT_C_CHANNEL_LIST); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CHANGE_CHANNEL& pkt) { return MakeSerializedPacket(pkt, PKT_C_CHANGE_CHANNEL); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_BUY_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_BUY_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_SELL_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_SELL_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_EQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_C_EQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_UNEQUIP_GEAR& pkt) { return MakeSerializedPacket(pkt, PKT_C_UNEQUIP_GEAR); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_USE_ITEM& pkt) { return MakeSerializedPacket(pkt, PKT_C_USE_ITEM); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_SET_QUICK_SLOT& pkt) { return MakeSerializedPacket(pkt, PKT_C_SET_QUICK_SLOT); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_USE_QUICK_SLOT& pkt) { return MakeSerializedPacket(pkt, PKT_C_USE_QUICK_SLOT); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CONNECT_LOBBY& pkt) { return MakeSerializedPacket(pkt, PKT_C_CONNECT_LOBBY); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_CREATE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_C_CREATE_CHARACTER); }
	static SendBufferRef MakeSerializedPacket(Protocol::C_DELETE_CHARACTER& pkt) { return MakeSerializedPacket(pkt, PKT_C_DELETE_CHARACTER); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	/** 헤더의 size(uint16)에 담기지 않는 패킷은 만들지 않고 nullptr를 돌려준다. Send는 nullptr를 버린다. */
	template<typename T>
	static SendBufferRef MakeSerializedPacket(T& pkt, uint16 pktId)
	{
		// 크기를 잘라 보내면 받는 쪽의 패킷 경계가 깨진다.
		const size_t bodySize = pkt.ByteSizeLong();
		if (bodySize > UINT16_MAX - sizeof(PacketHeader))
		{
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
			UE_LOG(LogP1Network, Error, TEXT("패킷 %d의 본문이 %llu바이트라 헤더의 크기에 담기지 않아 보내지 않는다"), pktId, static_cast<uint64>(bodySize));
#else
			GLogger->Error("패킷 {}의 본문이 {}바이트라 헤더의 크기에 담기지 않아 보내지 않는다", pktId, bodySize);
#endif
			return nullptr;
		}

		const int32 dataSize = static_cast<int32>(bodySize);
		const uint16 packetSize = static_cast<uint16>(bodySize + sizeof(PacketHeader));

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