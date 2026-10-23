// 채팅창이 보관하는 줄의 수 제한과, 받은 메시지를 이름과 함께 한 줄로 보여 주는 규칙을 고정한다.
//
// 왜 이것인가: 채팅은 게임 서버의 모든 룸에서 오므로 받는 쪽은 다른 룸의 플레이어를 엔티티로 모른다. 이름은 패킷에
// 실린 것을 그대로 쓰고, 내 줄인지는 내 엔티티 번호로 가른다. 플레이어가 쓴 글자가 리치 텍스트 태그로 읽히면
// 다른 플레이어의 채팅창에서 이름의 색이 바뀌거나 글자가 사라진다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Chat

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Game/Chat/P1ChatLimits.h"
#include "Game/Chat/P1ChatLog.h"
#include "Game/Chat/P1ChatPackets.h"
#include "Game/Chat/P1ChatSubsystem.h"
#include "UI/Screens/P1ChatLineWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1ChatTest
{
    constexpr uint64 MY_ENTITY_ID = 7;
    constexpr uint64 OTHER_ENTITY_ID = 8;

    FP1ChatLine MakeLine(EP1ChatLineKind Kind, const FString& Name, const FString& Message)
    {
        FP1ChatLine Line;
        Line.Kind = Kind;
        Line.Name = Name;
        Line.Message = Message;
        return Line;
    }

    Protocol::S_CHAT_MESSAGE MakeMessage(uint64 EntityId, const FString& Name, const FString& Message)
    {
        Protocol::S_CHAT_MESSAGE ChatMessagePkt;
        ChatMessagePkt.set_entity_id(static_cast<int64>(EntityId));
        ChatMessagePkt.set_name(TCHAR_TO_UTF8(*Name));
        ChatMessagePkt.set_message(TCHAR_TO_UTF8(*Message));
        return ChatMessagePkt;
    }

    // 게임 인스턴스 서브시스템은 바깥 객체가 게임 인스턴스여야 만들어진다.
    UP1ChatSubsystem* MakeChat()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UP1ChatSubsystem>(GameInstance);
    }

    FString Repeat(const TCHAR* Character, int32 Count)
    {
        FString Result;
        for (int32 Index = 0; Index < Count; ++Index)
            Result += Character;
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ChatLogTest,
    "P1.Chat.Log",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ChatLogTest::RunTest(const FString& Parameters)
{
    using namespace P1ChatTest;

    // 1) 정한 수를 넘으면 가장 오래된 줄부터 버리고, 남은 줄은 들어온 순서를 지킨다.
    {
        FP1ChatLog Log(3);
        for (int32 Index = 0; Index < 5; ++Index)
            Log.Add(MakeLine(EP1ChatLineKind::Other, TEXT("아린"), FString::FromInt(Index)));

        const TArray<FP1ChatLine>& Lines = Log.GetLines();
        if (TestEqual(TEXT("상한: 개수"), Lines.Num(), 3))
        {
            TestEqual(TEXT("상한: 가장 오래 남은 줄"), Lines[0].Message, FString(TEXT("2")));
            TestEqual(TEXT("상한: 가장 최근 줄"), Lines[2].Message, FString(TEXT("4")));
        }
    }

    // 2) 지우면 비고, 1보다 작은 상한은 1로 본다.
    {
        FP1ChatLog Log(0);
        TestEqual(TEXT("상한 하한"), Log.GetCapacity(), 1);
        Log.Add(MakeLine(EP1ChatLineKind::Other, TEXT("아린"), TEXT("하나")));
        Log.Add(MakeLine(EP1ChatLineKind::Other, TEXT("아린"), TEXT("둘")));
        if (TestEqual(TEXT("상한 하한: 개수"), Log.GetLines().Num(), 1))
            TestEqual(TEXT("상한 하한: 남은 줄"), Log.GetLines()[0].Message, FString(TEXT("둘")));

        Log.Clear();
        TestEqual(TEXT("지우기"), Log.GetLines().Num(), 0);
    }

    // 3) 채팅 서브시스템은 50줄까지 보관하고, 줄이 바뀔 때마다 알린다.
    {
        UP1ChatSubsystem* Chat = MakeChat();
        int32 ChangedCount = 0;
        Chat->OnChatLogChanged.AddLambda([&]() { ++ChangedCount; });

        for (int32 Index = 0; Index < 52; ++Index)
            Chat->HandleChatMessage(MakeMessage(OTHER_ENTITY_ID, TEXT("아린"), FString::FromInt(Index)), MY_ENTITY_ID);

        TestEqual(TEXT("서브시스템: 알림"), ChangedCount, 52);
        if (TestEqual(TEXT("서브시스템: 보관 상한"), Chat->GetLines().Num(), 50))
            TestEqual(TEXT("서브시스템: 가장 오래 남은 줄"), Chat->GetLines()[0].Message, FString(TEXT("2")));

        Chat->ClearLog();
        TestEqual(TEXT("서브시스템: 지우기"), Chat->GetLines().Num(), 0);
        TestEqual(TEXT("서브시스템: 지우기 알림"), ChangedCount, 53);
    }

    // 4) 서버가 거절하면 이름 없는 안내 줄이 남고, 받아들였으면 아무 줄도 남지 않는다.
    {
        UP1ChatSubsystem* Chat = MakeChat();

        Protocol::S_CHAT AcceptedPkt;
        AcceptedPkt.set_result(Protocol::CHAT_RESULT_SUCCESS);
        Chat->HandleChatResult(AcceptedPkt);
        TestEqual(TEXT("거절: 성공은 줄을 남기지 않음"), Chat->GetLines().Num(), 0);

        // 거절은 경고 로그를 남긴다. 테스트가 그 경고로 실패하지 않게 미리 알린다.
        AddExpectedMessage(TEXT("채팅이 거절됐습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

        Protocol::S_CHAT RejectedPkt;
        RejectedPkt.set_result(Protocol::CHAT_RESULT_INVALID_MESSAGE);
        Chat->HandleChatResult(RejectedPkt);
        if (TestEqual(TEXT("거절: 안내 줄"), Chat->GetLines().Num(), 1))
        {
            TestTrue(TEXT("거절: 종류"), Chat->GetLines()[0].Kind == EP1ChatLineKind::System);
            TestTrue(TEXT("거절: 이름 없음"), Chat->GetLines()[0].Name.IsEmpty());
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ChatPacketsTest,
    "P1.Chat.Packets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ChatPacketsTest::RunTest(const FString& Parameters)
{
    using namespace P1ChatTest;

    // 1) 받은 메시지는 실린 이름을 쓰고, 내 엔티티 번호와 같을 때만 내 줄이다.
    {
        const FP1ChatLine Other = P1ChatPackets::FromProto(MakeMessage(OTHER_ENTITY_ID, TEXT("아린"), TEXT("다들 던전 앞에 모여요")), MY_ENTITY_ID);
        TestTrue(TEXT("받기: 다른 플레이어"), Other.Kind == EP1ChatLineKind::Other);
        TestEqual(TEXT("받기: 이름"), Other.Name, FString(TEXT("아린")));
        TestEqual(TEXT("받기: 메시지"), Other.Message, FString(TEXT("다들 던전 앞에 모여요")));

        const FP1ChatLine Mine = P1ChatPackets::FromProto(MakeMessage(MY_ENTITY_ID, TEXT("검은늑대"), TEXT("저 지금 가요")), MY_ENTITY_ID);
        TestTrue(TEXT("받기: 내 줄"), Mine.Kind == EP1ChatLineKind::Mine);

        // 룸에 들어가기 전에는 내 엔티티 번호가 0이다. 번호가 0인 메시지를 내 줄로 읽지 않는다.
        const FP1ChatLine BeforeEnter = P1ChatPackets::FromProto(MakeMessage(0, TEXT("아린"), TEXT("안녕")), 0);
        TestTrue(TEXT("받기: 입장 전"), BeforeEnter.Kind == EP1ChatLineKind::Other);

        const FP1ChatLine NoName = P1ChatPackets::FromProto(MakeMessage(OTHER_ENTITY_ID, TEXT(""), TEXT("수고하셨습니다")), MY_ENTITY_ID);
        TestTrue(TEXT("받기: 이름이 없으면 비워 둠"), NoName.Name.IsEmpty());
    }

    // 2) 요청은 앞뒤 공백을 지운 문장으로 만들고, 비었거나 길이 제한을 넘으면 만들지 않는다.
    {
        const TOptional<Protocol::C_CHAT> Trimmed = P1ChatPackets::ToRequest(TEXT("  어디쯤이에요?  "));
        if (TestTrue(TEXT("요청: 보통 문장"), Trimmed.IsSet()))
            TestEqual(TEXT("요청: 앞뒤 공백 제거"), FString(UTF8_TO_TCHAR(Trimmed->message().c_str())), FString(TEXT("어디쯤이에요?")));

        TestFalse(TEXT("요청: 빈 문장"), P1ChatPackets::ToRequest(TEXT("")).IsSet());
        TestFalse(TEXT("요청: 공백만"), P1ChatPackets::ToRequest(TEXT("   ")).IsSet());

        // 서버처럼 UTF-16 글자로 센다. UTF-8 바이트로 세면 한글 33자에서 막힌다.
        TestTrue(TEXT("요청: 한도 글자 수의 한글"), P1ChatPackets::ToRequest(Repeat(TEXT("가"), P1ChatLimits::MAX_MESSAGE_LENGTH)).IsSet());
        TestFalse(TEXT("요청: 한도를 넘는 한글"), P1ChatPackets::ToRequest(Repeat(TEXT("가"), P1ChatLimits::MAX_MESSAGE_LENGTH + 1)).IsSet());
    }

    // 3) 비었거나 공백만 있는 문장은 보내지 않는다.
    {
        UP1ChatSubsystem* Chat = MakeChat();
        TestFalse(TEXT("보내기: 공백만"), Chat->SendChat(TEXT("  ")));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ChatLineMarkupTest,
    "P1.Chat.LineMarkup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ChatLineMarkupTest::RunTest(const FString& Parameters)
{
    using namespace P1ChatTest;

    // 1) 이름은 누가 말했는지에 따른 글자 스타일로, 메시지는 기본 스타일로 이름 뒤에 붙인다.
    TestEqual(TEXT("표시: 다른 플레이어"),
        UP1ChatLineWidget::MakeMarkup(MakeLine(EP1ChatLineKind::Other, TEXT("아린"), TEXT("오른쪽 길로 오세요"))),
        FString(TEXT("<Other>아린</>: 오른쪽 길로 오세요")));
    TestEqual(TEXT("표시: 내 줄"),
        UP1ChatLineWidget::MakeMarkup(MakeLine(EP1ChatLineKind::Mine, TEXT("검은늑대"), TEXT("도착했어요"))),
        FString(TEXT("<Mine>검은늑대</>: 도착했어요")));

    // 2) 이름을 받지 못한 줄은 대신하는 이름을, 안내 줄은 이름 없이 안내 스타일을 쓴다.
    TestEqual(TEXT("표시: 이름 없음"),
        UP1ChatLineWidget::MakeMarkup(MakeLine(EP1ChatLineKind::Other, TEXT(""), TEXT("수고하셨습니다"))),
        FString::Printf(TEXT("<Unknown>%s</>: 수고하셨습니다"), UP1ChatLineWidget::UNKNOWN_NAME));
    TestEqual(TEXT("표시: 안내"),
        UP1ChatLineWidget::MakeMarkup(MakeLine(EP1ChatLineKind::System, TEXT(""), TEXT("메시지를 보내지 못했습니다."))),
        FString(TEXT("<System>메시지를 보내지 못했습니다.</>")));

    // 3) 플레이어가 쓴 태그 글자는 글자 그대로 보이게 바꾼다. &를 먼저 바꿔 두 번 바뀌지 않는다.
    TestEqual(TEXT("표시: 이스케이프"),
        UP1ChatLineWidget::MakeMarkup(MakeLine(EP1ChatLineKind::Other, TEXT("<Mine>"), TEXT("</>a&lt;\""))),
        FString(TEXT("<Other>&lt;Mine&gt;</>: &lt;/&gt;a&amp;lt;&quot;")));

    return true;
}

#endif
