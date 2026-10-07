#include "Core/pch.h"
#include <gtest/gtest.h>

/*--------------------------------------------------------------
    패킷 디스패치 테스트

    받은 패킷의 id는 상대가 보낸 값이다. 핸들러 테이블은 UINT16_MAX 칸이라
    uint16의 최대값 65535는 테이블 밖이다. 이 id를 그대로 인덱스로 쓰면
    클라이언트 하나가 서버의 범위 밖 메모리를 함수로 부르게 만든다.

    디스패치 코드는 Protocol/Templates/PacketHandler.h에서 생성되고 클라이언트와
    DummyClient도 같은 코드를 받는다. 이 테스트는 GenPackets.bat을 다시 돌려도
    범위 확인이 남아 있는지 지킨다.
---------------------------------------------------------------*/

TEST(PacketDispatch, RejectsIdOutsideHandlerTable)
{
    ServerPacketHandler::Init();

    PacketHeader header;
    header.size = sizeof(PacketHeader);
    header.id = UINT16_MAX;

    PacketSessionRef session = nullptr;
    EXPECT_FALSE(ServerPacketHandler::HandlePacket(session, reinterpret_cast<BYTE*>(&header), sizeof(header)));
}

// 헤더보다 짧은 입력은 헤더를 읽기 전에 거절한다. 읽으면 받은 길이 밖의 바이트를 id로 쓰고,
// 본문 길이(len - 헤더 크기)가 음수가 된 채로 파서에 넘어간다.
TEST(PacketDispatch, RejectsInputShorterThanHeader)
{
    ServerPacketHandler::Init();

    // 실제 핸들러는 세션이 없으면 결과를 구별할 수 없으므로, 테이블 칸을 호출 기록으로 바꿔 끼운다.
    int32 dispatchedCount = 0;
    GPacketHandler[PKT_C_PING] = [&dispatchedCount](PacketSessionRef&, BYTE*, int32) { dispatchedCount++; return true; };

    PacketHeader header;
    header.size = 0;
    header.id = PKT_C_PING;

    PacketSessionRef session = nullptr;
    for (int32 len = 0; len < static_cast<int32>(sizeof(PacketHeader)); len++)
        EXPECT_FALSE(ServerPacketHandler::HandlePacket(session, reinterpret_cast<BYTE*>(&header), len)) << "len = " << len;

    EXPECT_EQ(dispatchedCount, 0);

    ServerPacketHandler::Init();
}
