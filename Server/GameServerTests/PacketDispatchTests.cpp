#include "pch.h"
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
