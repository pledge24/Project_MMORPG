#include "Core/pch.h"
#include <gtest/gtest.h>

/*--------------------------------------------------------------
    패킷 직렬화 왕복 테스트

    서버가 실제로 보내는 경로(ServerPacketHandler::MakeSerializedPacket)로
    직렬화한 뒤, 클라이언트가 실제로 받는 경로(PacketHeader 파싱 +
    ParseFromArray)로 되읽어 원본과 같은지 본다.

    protobuf 자체를 믿지 못해서가 아니라, 그 사이에 있는 손으로 짠 프레이밍
    (PacketHeader의 size/id 계산, 페이로드 오프셋)이 자동 생성물과 어긋나지
    않는지를 본다. GenPackets.bat 재실행 시 실제로 깨지는 지점이다.
---------------------------------------------------------------*/

namespace
{
    template <typename TPacket>
    void ExpectRoundTrip(TPacket& pkt, uint16 expectedId)
    {
        const string original = pkt.SerializeAsString();

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(pkt);
        ASSERT_TRUE(sendBuffer != nullptr);

        const PacketHeader* header = reinterpret_cast<const PacketHeader*>(sendBuffer->Buffer());
        EXPECT_EQ(header->id, expectedId);
        EXPECT_EQ(static_cast<int32>(header->size), sendBuffer->WriteSize());
        EXPECT_EQ(static_cast<size_t>(header->size), pkt.ByteSizeLong() + sizeof(PacketHeader));

        const int32 payloadSize = static_cast<int32>(header->size) - static_cast<int32>(sizeof(PacketHeader));
        TPacket parsed;
        ASSERT_TRUE(parsed.ParseFromArray(sendBuffer->Buffer() + sizeof(PacketHeader), payloadSize));
        EXPECT_EQ(parsed.SerializeAsString(), original);
    }
}

// 가변 길이 문자열 — 헤더의 size 계산이 페이로드 길이를 제대로 따라가는지
TEST(PacketSerialization, ChatRoundTrip)
{
    Protocol::S_CHAT pkt;
    pkt.set_entity_id(4242);
    pkt.set_msg("한글과 ASCII가 섞인 채팅 메시지");   // /utf-8 이므로 좁은 리터럴이 UTF-8 바이트다

    ExpectRoundTrip(pkt, PKT_S_CHAT);
}

// 중첩 메시지 + repeated — 페이로드 오프셋이 어긋나면 여기서 걸린다
TEST(PacketSerialization, MoveRoundTrip)
{
    Protocol::S_MOVE pkt;

    for (int32 i = 0; i < 3; i++)
    {
        Protocol::PosInfo* info = pkt.add_info();
        info->set_entity_id(1000 + i);
        info->set_yaw(90.5f * i);
        info->set_state(Protocol::MoveState::MOVE_STATE_RUN);

        Protocol::Vector* pos = info->mutable_pos();
        pos->set_x(1.5f * i);
        pos->set_y(-2.5f * i);
        pos->set_z(3.5f * i);
    }

    ExpectRoundTrip(pkt, PKT_S_MOVE);
}

/*--------------------------------------------------------------
    패킷 직렬화 크기 상한

    헤더의 size는 uint16이고 헤더를 포함한 길이다. 본문이 UINT16_MAX - 헤더 크기를 넘으면
    크기를 잘라 쓰는 대신 패킷을 만들지 않는다(TD-023). 잘린 크기가 나가면 받는 쪽의 패킷 경계가 깨진다.
---------------------------------------------------------------*/

namespace
{
    constexpr size_t MAX_BODY_SIZE = UINT16_MAX - sizeof(PacketHeader);

    /** 본문이 정확히 bodySize바이트인 S_CHAT. msg 길이가 16384 이상이면 태그 1바이트와 길이 3바이트가 붙는다. */
    Protocol::S_CHAT MakeChatWithBodySize(size_t bodySize)
    {
        Protocol::S_CHAT pkt;
        pkt.set_msg(string(bodySize - 4, 'a'));
        return pkt;
    }
}

TEST(PacketSerialization, LargestBodyFitsHeaderSize)
{
    Protocol::S_CHAT pkt = MakeChatWithBodySize(MAX_BODY_SIZE);
    ASSERT_EQ(pkt.ByteSizeLong(), MAX_BODY_SIZE) << "준비: 본문 크기를 상한에 맞추지 못했다";

    SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(pkt);

    ASSERT_NE(sendBuffer, nullptr);
    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(sendBuffer->Buffer());
    EXPECT_EQ(header->size, UINT16_MAX);
    EXPECT_EQ(sendBuffer->WriteSize(), static_cast<int32>(UINT16_MAX));
}

TEST(PacketSerialization, BodyLargerThanHeaderSizeIsNotSerialized)
{
    Protocol::S_CHAT pkt = MakeChatWithBodySize(MAX_BODY_SIZE + 1);
    ASSERT_EQ(pkt.ByteSizeLong(), MAX_BODY_SIZE + 1) << "준비: 본문 크기를 상한 + 1에 맞추지 못했다";

    EXPECT_EQ(ServerPacketHandler::MakeSerializedPacket(pkt), nullptr)
        << "헤더에 담기지 않는 크기를 잘라 보내면 받는 쪽의 패킷 경계가 깨진다";
}
