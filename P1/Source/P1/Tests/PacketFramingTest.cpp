// 클라이언트가 만드는 패킷의 바이트 배치를 고정한다.
//
// 왜 이것인가: 모든 송신이 MakeSerializedPacket 을 거친다. 여기서 헤더 크기나 필드 순서가
// 어긋나면 서버는 패킷을 잘못 읽고, 그 증상은 송신 지점이 아니라 서버 핸들러에서 드러난다.
// 이 배치는 클라와 서버가 함께 지켜야 하는 규약이라 한쪽만 고치면 바로 깨진다.
// 서버 쪽 짝은 GameServerTests 의 PacketSerialization 스위트다.
//
// SendBuffer 의 Append 와 Copy 는 대상으로 삼지 않았다. 클라이언트에서 부르는 곳이 없다
// (docs/tech-debt.md 참조).
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "ClientPacketHandler.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1PacketFramingTest,
    "P1.Network.PacketFraming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1PacketFramingTest::RunTest(const FString& Parameters)
{
    const int32 HeaderSize = static_cast<int32>(sizeof(PacketHeader));

    // 소스 파일 인코딩에 기대지 않도록 UTF-8 바이트를 직접 적는다. "안녕 서버" 다.
    const std::string Payload = "\xEC\x95\x88\xEB\x85\x95 \xEC\x84\x9C\xEB\xB2\x84";

    Protocol::C_CHAT Pkt;
    Pkt.set_msg(Payload);

    const int32 DataSize = static_cast<int32>(Pkt.ByteSizeLong());
    const int32 Expected = DataSize + HeaderSize;

    // 인자 하나짜리 공개 오버로드를 쓴다. 메시지 타입이 어느 패킷 id 로 가는지까지 함께 본다.
    auto Packet = ClientPacketHandler::MakeSerializedPacket(Pkt);

    // 1) 버퍼는 헤더와 본문에 딱 맞게 잡히고, Close 가 그 크기를 기록 위치에 반영한다.
    TestEqual(TEXT("버퍼 길이"), Packet->Len(), Expected);
    TestEqual(TEXT("Close 가 반영한 WriteSize"), Packet->WriteSize(), Expected);

    // 2) 헤더는 앞 4바이트에 size, id 순으로 실린다. 서버가 이 순서로 읽는다.
    TestEqual(TEXT("헤더 크기"), HeaderSize, 4);
    const PacketHeader* Header = reinterpret_cast<const PacketHeader*>(Packet->Buffer());
    TestEqual(TEXT("헤더의 size"), static_cast<int32>(Header->size), Expected);
    TestEqual(TEXT("헤더의 id"), static_cast<int32>(Header->id), static_cast<int32>(PKT_C_CHAT));

    // 3) 본문은 헤더 바로 뒤에서 시작하고 그대로 복원된다.
    Protocol::C_CHAT Parsed;
    const bool bParsed = Parsed.ParseFromArray(Packet->Buffer() + HeaderSize, DataSize);
    TestTrue(TEXT("본문이 파싱된다"), bParsed);
    TestEqual(TEXT("왕복한 msg 의 길이"), static_cast<int32>(Parsed.msg().size()),
              static_cast<int32>(Payload.size()));
    TestTrue(TEXT("왕복한 msg 의 내용"), Parsed.msg() == Payload);

    // 4) 빈 메시지는 헤더만 실려 나간다. 본문 0바이트가 경계다.
    Protocol::C_CHAT EmptyPkt;
    auto EmptyPacket = ClientPacketHandler::MakeSerializedPacket(EmptyPkt);
    TestEqual(TEXT("빈 패킷의 WriteSize"), EmptyPacket->WriteSize(), HeaderSize);

    const PacketHeader* EmptyHeader = reinterpret_cast<const PacketHeader*>(EmptyPacket->Buffer());
    TestEqual(TEXT("빈 패킷 헤더의 size"), static_cast<int32>(EmptyHeader->size), HeaderSize);
    TestEqual(TEXT("빈 패킷 헤더의 id"), static_cast<int32>(EmptyHeader->id),
              static_cast<int32>(PKT_C_CHAT));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
