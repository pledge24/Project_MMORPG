// 로비 서브시스템이 캐릭터 목록 응답과 생성·삭제 응답을 캐릭터 목록 사본에 반영하는 규칙을 고정한다.
//
// 왜 이것인가: 캐릭터 목록을 로그인 화면 위젯이 들고 있었고, 위젯이 응답을 해석해 목록을 고쳤다. 이제 사본은 로비
// 서브시스템이 갖고 위젯은 바뀌었다는 알림을 받아 다시 그리기만 한다. 생성 응답에는 캐릭터 번호만 오므로, 이름과 직업은
// 요청할 때 적어 둔 값을 써야 한다. UWorld 없이 빈 게임 인스턴스 위에서 돈다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Lobby.Subsystem

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Protocol.pb.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일과 한 번역 단위로 묶여도 도우미 이름이 겹치지 않게 파일 이름의 네임스페이스에 둔다.
namespace P1LobbySubsystemTest
{
    /** 전사 「가나」(번호 11, 레벨 7)와 마법사 「다라」(번호 12, 레벨 3)가 있고 슬롯이 4개인 목록 응답. */
    Protocol::S_LOGIN MakeCharacterList()
    {
        Protocol::S_LOGIN Pkt;
        Pkt.set_success(true);
        Pkt.set_character_slot_count(4);

        Protocol::CharacterOverview* Warrior = Pkt.add_characters();
        Warrior->set_character_id(11);
        Warrior->set_name(TCHAR_TO_UTF8(TEXT("가나")));
        Warrior->set_class_(Protocol::CLASS_TYPE_WARRIOR);
        Warrior->set_level(7);

        Protocol::CharacterOverview* Mage = Pkt.add_characters();
        Mage->set_character_id(12);
        Mage->set_name(TCHAR_TO_UTF8(TEXT("다라")));
        Mage->set_class_(Protocol::CLASS_TYPE_MAGE);
        Mage->set_level(3);

        return Pkt;
    }

    /** 송신을 세기만 하는 로비 서브시스템과, 알림 횟수. */
    struct FFixture
    {
        UP1LobbySubsystem* Lobby = nullptr;
        int32 SentCount = 0;
        int32 ChangedCount = 0;
        int32 EnteredCount = 0;
        TArray<FString> CreateFailures;
        int32 DeleteFailedCount = 0;

        FFixture()
        {
            // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
            UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            Lobby = NewObject<UP1LobbySubsystem>(GameInstance);
            Lobby->SetPacketSender([this](SendBufferRef) { ++SentCount; });

            Lobby->OnCharactersChanged.AddLambda([this]() { ++ChangedCount; });
            Lobby->OnLobbyEntered.AddLambda([this]() { ++EnteredCount; });
            Lobby->OnCreateCharacterFailed.AddLambda([this](const FString& Cause) { CreateFailures.Add(Cause); });
            Lobby->OnDeleteCharacterFailed.AddLambda([this]() { ++DeleteFailedCount; });
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

    // 2)의 실패한 목록 응답은 경고를 남긴다.
    AddExpectedMessage(TEXT("서버가 캐릭터 목록을 불러오지 못함"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

    // 1) 목록 응답이 오면 캐릭터와 슬롯 수를 사본에 옮기고, 로비에 들어왔다고 알린 뒤 목록이 바뀌었다고 알린다.
    {
        FFixture F;
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
        TestEqual(TEXT("로비 입장 알림"), F.EnteredCount, 1);
        TestEqual(TEXT("목록 변경 알림"), F.ChangedCount, 1);
    }

    // 2) 실패한 목록 응답은 사본을 바꾸지 않고 로비에 들어왔다고 알리지 않는다.
    {
        FFixture F;
        int32 FailedCount = 0;
        F.Lobby->OnCharacterListFailed.AddLambda([&FailedCount]() { ++FailedCount; });

        Protocol::S_LOGIN Failed;
        Failed.set_success(false);
        F.Lobby->HandleCharacterList(Failed);

        TestEqual(TEXT("실패 알림"), FailedCount, 1);
        TestEqual(TEXT("캐릭터 없음"), F.Lobby->GetCharacters().Num(), 0);
        TestEqual(TEXT("슬롯 수는 0"), F.Lobby->GetCharacterSlotCount(), 0);
        TestEqual(TEXT("로비 입장 알림 없음"), F.EnteredCount, 0);
        TestEqual(TEXT("목록 변경 알림 없음"), F.ChangedCount, 0);
    }

    // 3) 생성에 성공하면 요청할 때의 이름과 직업, 레벨 1로 목록 끝에 더한다. 응답에는 캐릭터 번호만 있다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->CreateCharacter(TEXT("마바"), EP1CharacterClass::Mage);
        TestEqual(TEXT("생성 요청 송신"), F.SentCount, 1);

        Protocol::S_CREATE_CHARACTER Created;
        Created.set_success(true);
        Created.set_character_id(13);
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

    // 4) 생성에 실패하면 목록을 바꾸지 않고 서버가 보낸 사유를 알린다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->CreateCharacter(TEXT("가나"), EP1CharacterClass::Warrior);

        Protocol::S_CREATE_CHARACTER Rejected;
        Rejected.set_success(false);
        Rejected.set_cause(TCHAR_TO_UTF8(TEXT("이미 있는 이름입니다.")));
        F.Lobby->HandleCreateCharacter(Rejected);

        TestEqual(TEXT("실패 뒤 캐릭터 수"), F.Lobby->GetCharacters().Num(), 2);
        if (TestEqual(TEXT("실패 알림 수"), F.CreateFailures.Num(), 1))
            TestEqual(TEXT("실패 사유"), F.CreateFailures[0], FString(TEXT("이미 있는 이름입니다.")));
        TestEqual(TEXT("목록 변경 알림은 목록 응답의 것뿐"), F.ChangedCount, 1);
    }

    // 5) 삭제에 성공하면 응답의 캐릭터 번호로 찾아 지운다. 실패하면 목록을 바꾸지 않고 실패를 알린다.
    {
        FFixture F;
        F.Lobby->HandleCharacterList(MakeCharacterList());
        F.Lobby->DeleteCharacter(11);
        TestEqual(TEXT("삭제 요청 송신"), F.SentCount, 1);

        Protocol::S_DELETE_CHARACTER Rejected;
        Rejected.set_success(false);
        F.Lobby->HandleDeleteCharacter(Rejected);
        TestEqual(TEXT("삭제 실패 알림"), F.DeleteFailedCount, 1);
        TestEqual(TEXT("삭제 실패 뒤 캐릭터 수"), F.Lobby->GetCharacters().Num(), 2);

        Protocol::S_DELETE_CHARACTER Deleted;
        Deleted.set_success(true);
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
    }

    return true;
}

#endif
