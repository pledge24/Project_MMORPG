// 로비 서브시스템이 로비 응답을 캐릭터 목록 사본에 반영하고, 실패를 결과 값의 화면 문구로 알리는 규칙을 고정한다.
//
// 왜 이것인가: 캐릭터 목록을 로그인 화면 위젯이 들고 있었고, 위젯이 응답을 해석해 목록을 고쳤다. 이제 사본은 로비
// 서브시스템이 갖고 위젯은 바뀌었다는 알림을 받아 다시 그리기만 한다. 서버는 실패를 화면 문구가 아니라 로비 결과 값으로
// 보내므로, 결과 값마다 클라이언트에 문구가 있어야 한다. 문구가 빠진 값은 「알 수 없는 오류」로 보인다.
// UWorld 없이 빈 게임 인스턴스 위에서 돈다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Lobby

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Lobby.pb.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1LobbySubsystemTest
{
    void SetCharacter(Protocol::LobbyCharacter* Character, int64 CharacterId, const TCHAR* Name, Protocol::CharacterClass CharacterClass, int32 Level)
    {
        Character->set_character_id(CharacterId);
        Character->set_name(TCHAR_TO_UTF8(Name));
        Character->set_character_class(CharacterClass);
        Character->set_level(Level);
    }

    /** 전사 「가나」(번호 11, 레벨 7)와 마법사 「다라」(번호 12, 레벨 3)가 있고 슬롯이 4개이며, 전사만 생성할 수 있는 목록. */
    Protocol::S_CHARACTER_LIST MakeCharacterList()
    {
        Protocol::S_CHARACTER_LIST Pkt;
        Pkt.set_character_slot_count(4);
        Pkt.add_creatable_classes(Protocol::CLASS_TYPE_WARRIOR);
        SetCharacter(Pkt.add_characters(), 11, TEXT("가나"), Protocol::CLASS_TYPE_WARRIOR, 7);
        SetCharacter(Pkt.add_characters(), 12, TEXT("다라"), Protocol::CLASS_TYPE_MAGE, 3);
        return Pkt;
    }

    /** 송신을 세기만 하는 로비 서브시스템과, 알림 횟수. */
    struct FFixture
    {
        UP1LobbySubsystem* Lobby = nullptr;
        int32 SentCount = 0;
        int32 ChangedCount = 0;
        int32 EnteredCount = 0;
        TArray<FText> ConnectFailures;
        TArray<FText> CreateFailures;
        TArray<FText> DeleteFailures;

        FFixture()
        {
            // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
            UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            Lobby = NewObject<UP1LobbySubsystem>(GameInstance);
            Lobby->SetPacketSender([this](SendBufferRef) { ++SentCount; });

            Lobby->OnCharactersChanged.AddLambda([this]() { ++ChangedCount; });
            Lobby->OnLobbyEntered.AddLambda([this]() { ++EnteredCount; });
            Lobby->OnLobbyConnectFailed.AddLambda([this](const FText& Message) { ConnectFailures.Add(Message); });
            Lobby->OnCreateCharacterFailed.AddLambda([this](const FText& Message) { CreateFailures.Add(Message); });
            Lobby->OnDeleteCharacterFailed.AddLambda([this](const FText& Message) { DeleteFailures.Add(Message); });
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1LobbySubsystemTest,
    "P1.Lobby.Subsystem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1LobbySubsystemTest::RunTest(const FString& Parameters)
{
    using namespace P1LobbySubsystemTest;

    // 2)의 거절된 로비 접속은 경고를 남긴다.
    AddExpectedMessage(TEXT("서버가 로비 접속을 거절함"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

    // 7)의 모르는 직업은 경고를 남긴다.
    AddExpectedMessage(TEXT("모르는 직업을 생성할 수 있는 직업에서 뺌"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

    // 1) 로비 접속이 성공하면 목록이 뒤따른다. 목록을 받으면 캐릭터와 슬롯 수를 사본에 옮기고,
    //    로비에 들어왔다고 알린 뒤 목록이 바뀌었다고 알린다.
    {
        FFixture F;
        F.Lobby->ConnectLobby(TEXT("ticket"));
        TestEqual(TEXT("로비 접속 송신"), F.SentCount, 1);

        Protocol::S_CONNECT_LOBBY Connected;
        Connected.set_result(Protocol::LOBBY_RESULT_SUCCESS);
        F.Lobby->HandleConnectLobby(Connected);
        TestEqual(TEXT("목록 전에는 로비 입장 알림 없음"), F.EnteredCount, 0);
        TestEqual(TEXT("성공은 실패 알림 없음"), F.ConnectFailures.Num(), 0);

        F.Lobby->HandleCharacterList(MakeCharacterList());

        TestEqual(TEXT("슬롯 수"), F.Lobby->GetCharacterSlotCount(), 4);
        const TArray<FP1LobbyCharacter>& Characters = F.Lobby->GetCharacters();
        if (TestEqual(TEXT("캐릭터 수"), Characters.Num(), 2))
        {
            TestEqual(TEXT("첫 캐릭터 번호"), Characters[0].CharacterId, 11LL);
            TestEqual(TEXT("첫 캐릭터 이름"), Characters[0].CharacterName, FString(TEXT("가나")));
            TestTrue(TEXT("첫 캐릭터 직업"), Characters[0].CharacterClass == EP1CharacterClass::Warrior);
            TestEqual(TEXT("첫 캐릭터 레벨"), Characters[0].CharacterLevel, 7);
            TestTrue(TEXT("둘째 캐릭터 직업"), Characters[1].CharacterClass == EP1CharacterClass::Mage);
        }
        const TArray<EP1CharacterClass>& CreatableClasses = F.Lobby->GetCreatableClasses();
        if (TestEqual(TEXT("생성할 수 있는 직업 수"), CreatableClasses.Num(), 1))
            TestTrue(TEXT("생성할 수 있는 직업은 서버가 알려 준 전사뿐"), CreatableClasses[0] == EP1CharacterClass::Warrior);
        TestEqual(TEXT("로비 입장 알림"), F.EnteredCount, 1);
        TestEqual(TEXT("목록 변경 알림"), F.ChangedCount, 1);
    }

    // 2) 거절된 로비 접속은 결과 값의 문구를 알리고, 사본을 바꾸지 않고 로비에 들어왔다고 알리지 않는다.
    {
        FFixture F;
        Protocol::S_CONNECT_LOBBY Rejected;
        Rejected.set_result(Protocol::LOBBY_RESULT_INVALID_AUTH_TICKET);
        F.Lobby->HandleConnectLobby(Rejected);

        if (TestEqual(TEXT("실패 알림 수"), F.ConnectFailures.Num(), 1))
        {
            TestTrue(TEXT("실패 문구는 결과 값의 문구"),
                F.ConnectFailures[0].EqualTo(UP1LobbySubsystem::GetResultText(Protocol::LOBBY_RESULT_INVALID_AUTH_TICKET)));
        }
        TestEqual(TEXT("슬롯 수는 0"), F.Lobby->GetCharacterSlotCount(), 0);
        TestEqual(TEXT("로비 입장 알림 없음"), F.EnteredCount, 0);
    }

    // 3) 생성에 성공하면 응답이 실은 캐릭터를 목록 끝에 더한다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->CreateCharacter(TEXT("마바"), EP1CharacterClass::Mage);
        TestEqual(TEXT("생성 요청 송신"), F.SentCount, 1);

        Protocol::S_CREATE_CHARACTER Created;
        Created.set_result(Protocol::LOBBY_RESULT_SUCCESS);
        SetCharacter(Created.mutable_character(), 13, TEXT("마바"), Protocol::CLASS_TYPE_MAGE, 1);
        F.Lobby->HandleCreateCharacter(Created);

        const TArray<FP1LobbyCharacter>& Characters = F.Lobby->GetCharacters();
        if (TestEqual(TEXT("생성 뒤 캐릭터 수"), Characters.Num(), 3))
        {
            TestEqual(TEXT("새 캐릭터 번호"), Characters[2].CharacterId, 13LL);
            TestEqual(TEXT("새 캐릭터 이름"), Characters[2].CharacterName, FString(TEXT("마바")));
            TestTrue(TEXT("새 캐릭터 직업"), Characters[2].CharacterClass == EP1CharacterClass::Mage);
            TestEqual(TEXT("새 캐릭터 레벨"), Characters[2].CharacterLevel, 1);
        }
        TestEqual(TEXT("목록 변경 알림"), F.ChangedCount, 2);
    }

    // 4) 생성에 실패하면 목록을 바꾸지 않고 결과 값의 문구를 알린다. 이름 중복과 빈 슬롯 없음은 서로 다른 문구다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());

        Protocol::S_CREATE_CHARACTER NameTaken;
        NameTaken.set_result(Protocol::LOBBY_RESULT_NAME_TAKEN);
        F.Lobby->HandleCreateCharacter(NameTaken);

        Protocol::S_CREATE_CHARACTER NoEmptySlot;
        NoEmptySlot.set_result(Protocol::LOBBY_RESULT_NO_EMPTY_SLOT);
        F.Lobby->HandleCreateCharacter(NoEmptySlot);

        TestEqual(TEXT("실패 뒤 캐릭터 수"), F.Lobby->GetCharacters().Num(), 2);
        if (TestEqual(TEXT("실패 알림 수"), F.CreateFailures.Num(), 2))
        {
            TestTrue(TEXT("이름 중복 문구"), F.CreateFailures[0].EqualTo(UP1LobbySubsystem::GetResultText(Protocol::LOBBY_RESULT_NAME_TAKEN)));
            TestTrue(TEXT("빈 슬롯 없음 문구"), F.CreateFailures[1].EqualTo(UP1LobbySubsystem::GetResultText(Protocol::LOBBY_RESULT_NO_EMPTY_SLOT)));
            TestFalse(TEXT("두 실패의 문구가 다르다"), F.CreateFailures[0].EqualTo(F.CreateFailures[1]));
        }
        TestEqual(TEXT("목록 변경 알림은 목록 응답의 것뿐"), F.ChangedCount, 1);
    }

    // 5) 삭제에 성공하면 응답의 캐릭터 번호로 찾아 지운다. 실패하면 목록을 바꾸지 않고 결과 값의 문구를 알린다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->DeleteCharacter(11);
        TestEqual(TEXT("삭제 요청 송신"), F.SentCount, 1);

        Protocol::S_DELETE_CHARACTER Rejected;
        Rejected.set_result(Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND);
        Rejected.set_character_id(11);
        F.Lobby->HandleDeleteCharacter(Rejected);
        if (TestEqual(TEXT("삭제 실패 알림"), F.DeleteFailures.Num(), 1))
            TestTrue(TEXT("삭제 실패 문구"), F.DeleteFailures[0].EqualTo(UP1LobbySubsystem::GetResultText(Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND)));
        TestEqual(TEXT("삭제 실패 뒤 캐릭터 수"), F.Lobby->GetCharacters().Num(), 2);

        Protocol::S_DELETE_CHARACTER Deleted;
        Deleted.set_result(Protocol::LOBBY_RESULT_SUCCESS);
        Deleted.set_character_id(11);
        F.Lobby->HandleDeleteCharacter(Deleted);

        const TArray<FP1LobbyCharacter>& Characters = F.Lobby->GetCharacters();
        if (TestEqual(TEXT("삭제 뒤 캐릭터 수"), Characters.Num(), 1))
            TestEqual(TEXT("남은 캐릭터 번호"), Characters[0].CharacterId, 12LL);
        TestEqual(TEXT("목록 변경 알림"), F.ChangedCount, 2);
    }

    // 6) 연결이 끊겨 로비를 떠나면 사본을 비운다. 다음 로비 접속이 목록을 다시 채운다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->Reset();

        TestEqual(TEXT("비운 뒤 캐릭터 수"), F.Lobby->GetCharacters().Num(), 0);
        TestEqual(TEXT("비운 뒤 슬롯 수"), F.Lobby->GetCharacterSlotCount(), 0);
        TestEqual(TEXT("비운 뒤 생성할 수 있는 직업 수"), F.Lobby->GetCreatableClasses().Num(), 0);
    }

    // 7) 서버가 이 클라이언트에 없는 직업을 생성할 수 있다고 알리면, 그 직업만 빼고 나머지는 서버의 순서대로 둔다.
    {
        FFixture F;
        Protocol::S_CHARACTER_LIST Pkt;
        Pkt.set_character_slot_count(4);
        Pkt.add_creatable_classes(Protocol::CLASS_TYPE_MAGE);
        Pkt.add_creatable_classes(static_cast<Protocol::CharacterClass>(99));
        Pkt.add_creatable_classes(Protocol::CLASS_TYPE_WARRIOR);
        F.Lobby->HandleCharacterList(Pkt);

        const TArray<EP1CharacterClass>& CreatableClasses = F.Lobby->GetCreatableClasses();
        if (TestEqual(TEXT("모르는 직업을 뺀 수"), CreatableClasses.Num(), 2))
        {
            TestTrue(TEXT("첫 직업은 마법사"), CreatableClasses[0] == EP1CharacterClass::Mage);
            TestTrue(TEXT("둘째 직업은 전사"), CreatableClasses[1] == EP1CharacterClass::Warrior);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1LobbyResultTextTest,
    "P1.Lobby.ResultText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1LobbyResultTextTest::RunTest(const FString& Parameters)
{
    // 생성된 정의의 값 범위를 돈다. 스키마에 결과 값을 더하고 문구를 잊으면 그 값이 「알 수 없는 오류」로 걸린다.
    const FText UnknownText = UP1LobbySubsystem::GetResultText(static_cast<Protocol::LobbyResult>(Protocol::LobbyResult_MAX + 1));

    TArray<FString> Texts;
    for (int32 Value = Protocol::LobbyResult_MIN; Value <= Protocol::LobbyResult_MAX; Value++)
    {
        if (Protocol::LobbyResult_IsValid(Value) == false)
            continue;

        const Protocol::LobbyResult Result = static_cast<Protocol::LobbyResult>(Value);
        const FString Name = UTF8_TO_TCHAR(Protocol::LobbyResult_Name(Result).c_str());
        const FText Text = UP1LobbySubsystem::GetResultText(Result);

        TestFalse(FString::Printf(TEXT("%s의 문구가 있다"), *Name), Text.IsEmpty());
        TestFalse(FString::Printf(TEXT("%s의 문구가 모르는 값의 문구가 아니다"), *Name), Text.EqualTo(UnknownText));
        TestFalse(FString::Printf(TEXT("%s의 문구가 다른 값과 겹치지 않는다"), *Name), Texts.Contains(Text.ToString()));
        Texts.Add(Text.ToString());
    }

    TestTrue(TEXT("결과 값을 하나 이상 확인했다"), Texts.Num() > 1);
    TestFalse(TEXT("모르는 값에도 문구가 있다"), UnknownText.IsEmpty());

    return true;
}

#endif
