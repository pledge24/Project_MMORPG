#pragma once
#include "Protocol.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
#include "Network/SendBuffer.h"
#include "Utils/Types.h"
#include "Utils/LogCategory.h"
#endif

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Auto-generated
enum : uint16
{
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);

// Auto-generated template Handle Functions
{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(PacketSessionRef& session, Protocol::{{pkt.name}}& pkt);
{%- endfor %}

class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		// Auto-generated
{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, session, buffer, len); };
{%- endfor %}
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

	// Auto-generated
{%- for pkt in parser.send_pkt %}
	static SendBufferRef MakeSerializedPacket(Protocol::{{pkt.name}}& pkt) { return MakeSerializedPacket(pkt, PKT_{{pkt.name}}); }
{%- endfor %}

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
