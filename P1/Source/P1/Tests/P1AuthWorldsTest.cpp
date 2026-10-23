// 인증 클라이언트가 인증 서버의 월드 목록 JSON을 월드 목록 사본으로 읽는 규칙을 고정한다.
//
// 왜 이것인가: 월드 목록은 게임 서버가 Redis에 쓰고 인증 서버가 그대로 전달한다. 혼잡도는 세 이름(SMOOTH, NORMAL,
// CROWDED) 가운데 하나이고, 서버가 새 이름을 더하거나 항목 형식이 어긋나면 그 항목만 빼고 나머지 월드는 보여야 한다.
// 항목 하나 때문에 목록 전체가 비면 아무 월드에도 들어가지 못한다. HTTP 없이 JSON만 넣어 본다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Auth.Worlds

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Auth/P1AuthClient.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1AuthWorldsTest
{
    TArray<TSharedPtr<FJsonValue>> ReadArray(const TCHAR* Json)
    {
        TArray<TSharedPtr<FJsonValue>> Values;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
        FJsonSerializer::Deserialize(Reader, Values);
        return Values;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1AuthWorldsTest,
    "P1.Auth.Worlds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1AuthWorldsTest::RunTest(const FString& Parameters)
{
    using namespace P1AuthWorldsTest;

    // 3)의 버린 항목 다섯 개는 경고를 남긴다.
    AddExpectedMessage(TEXT("월드 목록에서 읽지 못한 항목을 뺀다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 5, false);

    // 1) 인증 서버가 보낸 순서대로 번호, 이름, 혼잡도를 읽는다.
    {
        const TArray<FP1World> Worlds = UP1AuthClient::ParseWorlds(ReadArray(TEXT(R"([
            { "worldId": 1, "name": "하나", "congestion": "SMOOTH" },
            { "worldId": 2, "name": "둘", "congestion": "NORMAL" },
            { "worldId": 3, "name": "셋", "congestion": "CROWDED" }
        ])")));

        if (TestEqual(TEXT("월드 수"), Worlds.Num(), 3))
        {
            TestEqual(TEXT("첫 월드 번호"), Worlds[0].WorldId, 1);
            TestEqual(TEXT("첫 월드 이름"), Worlds[0].Name, FString(TEXT("하나")));
            TestTrue(TEXT("원활"), Worlds[0].Congestion == EP1WorldCongestion::Smooth);
            TestTrue(TEXT("보통"), Worlds[1].Congestion == EP1WorldCongestion::Normal);
            TestTrue(TEXT("혼잡"), Worlds[2].Congestion == EP1WorldCongestion::Crowded);
            TestEqual(TEXT("마지막 월드 번호"), Worlds[2].WorldId, 3);
        }
    }

    // 2) 빈 목록은 빈 목록이다.
    {
        TestEqual(TEXT("빈 목록"), UP1AuthClient::ParseWorlds(ReadArray(TEXT("[]"))).Num(), 0);
    }

    // 3) 형식이 틀린 항목만 빼고 나머지는 남긴다.
    {
        const TArray<FP1World> Worlds = UP1AuthClient::ParseWorlds(ReadArray(TEXT(R"([
            { "worldId": 1, "name": "하나", "congestion": "FULL" },
            { "worldId": 0, "name": "영", "congestion": "SMOOTH" },
            { "name": "번호 없음", "congestion": "SMOOTH" },
            { "worldId": 4, "congestion": "SMOOTH" },
            "월드가 아님",
            { "worldId": 7, "name": "일곱", "congestion": "NORMAL" }
        ])")));

        if (TestEqual(TEXT("읽은 월드 수"), Worlds.Num(), 1))
            TestEqual(TEXT("남은 월드"), Worlds[0].WorldId, 7);
    }

    return true;
}

#endif
