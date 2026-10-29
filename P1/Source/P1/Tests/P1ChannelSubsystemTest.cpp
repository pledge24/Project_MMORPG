// 채널 서브시스템이 지금 채널, 채널 목록, 채널 변경 요청의 대기를 서버 응답에 맞춰 바꾸는 규칙을 고정한다.
//
// 왜 이것인가: 룸이 맵 하나의 사본이 되면서 같은 맵에 룸이 채널 수만큼 생겼다. 클라이언트는 룸 입장 응답이 실은 채널을
// 지금 채널로 보이고, 채널 변경은 응답이 올 때까지 다시 보내지 않는다. 지금 채널로 옮기는 요청은 서버가 거절하므로 보내지 않는다.
// 거절 사유마다 화면 문구가 있어야 한다. 문구가 빠진 값은 「알 수 없는 오류」로 보인다.
// UWorld 없이 빈 게임 인스턴스 위에서 돈다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Channel

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Game/Channel/P1ChannelSubsystem.h"
#include "Entry.pb.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1ChannelSubsystemTest
{
    Protocol::S_ENTER_ROOM MakeEnterRoom(int32 Channel)
    {
        Protocol::S_ENTER_ROOM Pkt;
        Pkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        Pkt.mutable_my_entity()->set_entity_id(100);
        Pkt.set_channel(Channel);
        return Pkt;
    }

    Protocol::S_CHANGE_CHANNEL MakeChangeChannel(Protocol::EntryResult Result, int32 Channel = 0)
    {
        Protocol::S_CHANGE_CHANNEL Pkt;
        Pkt.set_result(Result);
        Pkt.set_channel(Channel);
        return Pkt;
    }

    /** 송신을 세기만 하는 채널 서브시스템과, 알림 기록. */
    struct FFixture
    {
        UP1ChannelSubsystem* Channels = nullptr;
        int32 SentCount = 0;
        TArray<int32> ChangedChannels;
        int32 ListChangedCount = 0;
        TArray<FText> ChangeFailures;

        FFixture()
        {
            // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
            UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            Channels = NewObject<UP1ChannelSubsystem>(GameInstance);
            Channels->SetPacketSender([this](SendBufferRef) { ++SentCount; });

            Channels->OnChannelChanged.AddLambda([this](int32 Channel) { ChangedChannels.Add(Channel); });
            Channels->OnChannelListChanged.AddLambda([this]() { ++ListChangedCount; });
            Channels->OnChannelChangeFailed.AddLambda([this](const FText& Message) { ChangeFailures.Add(Message); });
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1ChannelSubsystemTest,
    "P1.Channel.Subsystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1ChannelSubsystemTest::RunTest(const FString& Parameters)
{
    using namespace P1ChannelSubsystemTest;

    // 3)의 거절된 채널 변경은 경고를 남긴다.
    AddExpectedMessage(TEXT("채널 변경이 거절됐습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

    // 1) 룸 입장 응답의 채널이 지금 채널이다. 같은 룸 안의 순간이동처럼 채널이 그대로면 다시 알리지 않는다.
    {
        FFixture F;
        TestEqual(TEXT("입장 전에는 채널 없음"), F.Channels->GetChannel(), 0);

        F.Channels->HandleEnterRoom(MakeEnterRoom(2));
        F.Channels->HandleEnterRoom(MakeEnterRoom(2));

        TestEqual(TEXT("지금 채널"), F.Channels->GetChannel(), 2);
        if (TestEqual(TEXT("채널 변경 알림은 한 번"), F.ChangedChannels.Num(), 1))
            TestEqual(TEXT("알린 채널"), F.ChangedChannels[0], 2);
    }

    // 2) 채널 변경은 응답이 올 때까지 하나만 보낸다. 지금 채널로 옮기는 요청은 보내지 않는다.
    //    성공 응답 뒤의 룸 입장 응답이 대기를 풀고 지금 채널을 바꾼다.
    {
        FFixture F;
        F.Channels->HandleEnterRoom(MakeEnterRoom(1));

        TestFalse(TEXT("지금 채널로는 보내지 않음"), F.Channels->RequestChangeChannel(1));
        TestEqual(TEXT("지금 채널 요청 송신 없음"), F.SentCount, 0);

        TestTrue(TEXT("다른 채널 요청은 보냄"), F.Channels->RequestChangeChannel(2));
        TestTrue(TEXT("응답 대기 중"), F.Channels->IsChangingChannel());
        TestFalse(TEXT("대기 중의 두 번째 요청은 보내지 않음"), F.Channels->RequestChangeChannel(2));
        TestEqual(TEXT("송신은 한 번"), F.SentCount, 1);

        F.Channels->HandleChangeChannel(MakeChangeChannel(Protocol::ENTRY_RESULT_SUCCESS, 2));
        TestTrue(TEXT("성공 응답만으로는 아직 옮기지 않음"), F.Channels->IsChangingChannel());
        TestEqual(TEXT("룸 입장 전의 지금 채널"), F.Channels->GetChannel(), 1);

        F.Channels->HandleEnterRoom(MakeEnterRoom(2));
        TestFalse(TEXT("룸 입장 응답이 대기를 품"), F.Channels->IsChangingChannel());
        TestEqual(TEXT("옮긴 채널"), F.Channels->GetChannel(), 2);
    }

    // 3) 거절되면 결과 값의 문구를 알리고 대기를 풀어 다시 보낼 수 있게 한다.
    {
        FFixture F;
        F.Channels->HandleEnterRoom(MakeEnterRoom(1));
        F.Channels->RequestChangeChannel(2);

        F.Channels->HandleChangeChannel(MakeChangeChannel(Protocol::ENTRY_RESULT_DEAD));

        if (TestEqual(TEXT("실패 알림 수"), F.ChangeFailures.Num(), 1))
            TestTrue(TEXT("실패 문구는 결과 값의 문구"), F.ChangeFailures[0].EqualTo(UP1ChannelSubsystem::GetResultText(Protocol::ENTRY_RESULT_DEAD)));
        TestFalse(TEXT("거절되면 대기를 품"), F.Channels->IsChangingChannel());
        TestEqual(TEXT("채널은 그대로"), F.Channels->GetChannel(), 1);
        TestTrue(TEXT("다시 보낼 수 있음"), F.Channels->RequestChangeChannel(2));
    }

    // 4) 채널 목록 요청을 보내고, 받은 목록을 채널 순서 그대로 보관한 뒤 알린다.
    {
        FFixture F;
        F.Channels->RequestChannelList();
        TestEqual(TEXT("목록 요청 송신"), F.SentCount, 1);

        Protocol::S_CHANNEL_LIST Pkt;
        Protocol::ChannelInfo* First = Pkt.add_channels();
        First->set_channel(1);
        First->set_player_count(3);
        Protocol::ChannelInfo* Second = Pkt.add_channels();
        Second->set_channel(2);
        Second->set_player_count(0);
        F.Channels->HandleChannelList(Pkt);

        const TArray<FP1ChannelEntry>& Entries = F.Channels->GetChannels();
        if (TestEqual(TEXT("채널 수"), Entries.Num(), 2))
        {
            TestEqual(TEXT("첫 채널 번호"), Entries[0].Channel, 1);
            TestEqual(TEXT("첫 채널 인원"), Entries[0].PlayerCount, 3);
            TestEqual(TEXT("둘째 채널 번호"), Entries[1].Channel, 2);
            TestEqual(TEXT("둘째 채널 인원"), Entries[1].PlayerCount, 0);
        }
        TestEqual(TEXT("목록 변경 알림"), F.ListChangedCount, 1);
    }

    // 5) 채널 변경의 거절 사유마다 문구가 있다.
    {
        for (Protocol::EntryResult Result : { Protocol::ENTRY_RESULT_DEAD, Protocol::ENTRY_RESULT_MAP_LOADING, Protocol::ENTRY_RESULT_SAME_CHANNEL,
            Protocol::ENTRY_RESULT_INVALID_CHANNEL, Protocol::ENTRY_RESULT_INTERNAL_ERROR })
        {
            const FText Text = UP1ChannelSubsystem::GetResultText(Result);
            const FString Name = UTF8_TO_TCHAR(Protocol::EntryResult_Name(Result).c_str());
            TestFalse(*FString::Printf(TEXT("%s의 문구가 있음"), *Name), Text.IsEmpty());
            TestFalse(*FString::Printf(TEXT("%s는 알 수 없는 오류가 아님"), *Name), Text.ToString().StartsWith(TEXT("알 수 없는 오류")));
        }
    }

    return true;
}

#endif
