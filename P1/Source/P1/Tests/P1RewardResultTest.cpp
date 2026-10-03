// 보상 결과(S_REWARD_RESULT)를 내 플레이어 데이터에 반영하는 규칙을 고정한다.
//
// 왜 이것인가: 클라이언트는 보상 결과를 받고도 버렸다(#137). 플레이어가 몬스터를 처치하는 경로가 아직 없어
// PIE로는 보상을 받을 수 없으므로, 반영 결과를 이 테스트로만 확인한다.
//
// 경험치와 레벨은 서버가 계산한다. 클라이언트는 실려 온 최종 값을 사본에 쓰고 알리기만 한다. 기대값은 서버의
// PlayerMultiLevelUpTest와 같은 경우(레벨마다 요구 경험치 100, 200, 300)에서 손으로 적었다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Progress.RewardResult

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    /** 레벨 1, 경험치 0/100, 최대 HP 100, 골드 1000인 내 플레이어 데이터를 만든다. */
    UP1MyPlayerData* MakeLevelOneData()
    {
        // 게임 인스턴스 서브시스템은 게임 인스턴스 안에서만 만들 수 있다. 초기화하지 않은 빈 인스턴스면 된다.
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        UP1MyPlayerData* Data = NewObject<UP1MyPlayerData>(GameInstance);
        Data->ApplyLevel(1);
        Data->ApplyStat(Protocol::STAT_TYPE_MAX_EXP, 100);
        Data->ApplyStat(Protocol::STAT_TYPE_EXP, 0);
        Data->ApplyStat(Protocol::STAT_TYPE_MAX_HP, 100);
        Data->ApplyGold(1000);
        return Data;
    }

    Protocol::S_REWARD_RESULT MakeReward(int64 UpdatedExp, int64 UpdatedGold)
    {
        Protocol::S_REWARD_RESULT Pkt;
        Pkt.set_type(Protocol::REWARD_TYPE_MONSTER_KILL);
        Pkt.set_updated_exp(UpdatedExp);
        Pkt.set_updated_gold(UpdatedGold);
        return Pkt;
    }

    void AddLevelUp(Protocol::S_REWARD_RESULT& Pkt, int32 OldLevel, int32 NewLevel, int64 MaxExp, int64 MaxHp)
    {
        Pkt.set_is_level_up(true);
        Protocol::LevelUpInfo* Info = Pkt.mutable_level_up_details();
        Info->set_old_level(OldLevel);
        Info->set_new_level(NewLevel);

        Protocol::Stat* MaxExpStat = Info->add_updated_stat();
        MaxExpStat->set_type(Protocol::STAT_TYPE_MAX_EXP);
        MaxExpStat->set_value(MaxExp);

        Protocol::Stat* MaxHpStat = Info->add_updated_stat();
        MaxHpStat->set_type(Protocol::STAT_TYPE_MAX_HP);
        MaxHpStat->set_value(MaxHp);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1RewardResultTest,
    "P1.Progress.RewardResult",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1RewardResultTest::RunTest(const FString& Parameters)
{
    // 1) 레벨이 오르지 않으면 경험치만 쌓이고 레벨은 알리지 않는다.
    {
        UP1MyPlayerData* Data = MakeLevelOneData();

        int32 LevelBroadcasts = 0;
        Data->OnLevelChanged.AddLambda([&LevelBroadcasts](int32) { ++LevelBroadcasts; });

        TArray<int64> NotifiedExp;
        Data->OnStatChangedMappings[Protocol::STAT_TYPE_EXP].AddLambda([&NotifiedExp](int64 Value) { NotifiedExp.Add(Value); });

        Data->HandleRewardResult(MakeReward(70, 1000));

        TestEqual(TEXT("경험치 누적: 레벨"), Data->GetPlayerLevel(), 1);
        TestEqual(TEXT("경험치 누적: 레벨 알림 없음"), LevelBroadcasts, 0);
        TestEqual(TEXT("경험치 누적: 경험치"), Data->GetStatValue(Protocol::STAT_TYPE_EXP), 70LL);
        TestEqual(TEXT("경험치 누적: 최대 경험치"), Data->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), 100LL);
        TestTrue(TEXT("경험치 누적: 경험치 알림"), NotifiedExp.Num() == 1 && NotifiedExp[0] == 70);
    }

    // 2) 여러 레벨이 한 번에 오르면 새 레벨을 한 번 알리고, 레벨업 스탯을 경험치보다 먼저 반영한다.
    {
        UP1MyPlayerData* Data = MakeLevelOneData();

        TArray<int32> NotifiedLevels;
        Data->OnLevelChanged.AddLambda([&NotifiedLevels](int32 Level) { NotifiedLevels.Add(Level); });

        // 경험치를 알릴 때 HUD는 최대 경험치와 함께 막대를 그린다. 그때 이미 새 최대 경험치여야 한다.
        int64 MaxExpWhenExpNotified = 0;
        Data->OnStatChangedMappings[Protocol::STAT_TYPE_EXP].AddLambda([Data, &MaxExpWhenExpNotified](int64)
        {
            MaxExpWhenExpNotified = Data->GetStatValue(Protocol::STAT_TYPE_MAX_EXP);
        });

        // 1→2에 100, 2→3에 200을 쓰고 50이 남는다.
        Protocol::S_REWARD_RESULT Pkt = MakeReward(50, 1000);
        AddLevelUp(Pkt, 1, 3, 300, 140);
        Data->HandleRewardResult(Pkt);

        TestEqual(TEXT("여러 레벨: 레벨"), Data->GetPlayerLevel(), 3);
        TestTrue(TEXT("여러 레벨: 새 레벨을 한 번 알림"), NotifiedLevels.Num() == 1 && NotifiedLevels[0] == 3);
        TestEqual(TEXT("여러 레벨: 경험치"), Data->GetStatValue(Protocol::STAT_TYPE_EXP), 50LL);
        TestEqual(TEXT("여러 레벨: 최대 경험치"), Data->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), 300LL);
        TestEqual(TEXT("여러 레벨: 최대 HP"), Data->GetStatValue(Protocol::STAT_TYPE_MAX_HP), 140LL);
        TestEqual(TEXT("여러 레벨: 경험치를 알릴 때의 최대 경험치"), MaxExpWhenExpNotified, 300LL);
    }

    // 3) 골드는 실려 온 값으로 바꾸고, 알릴 때 이미 사본에 써 두었다.
    {
        UP1MyPlayerData* Data = MakeLevelOneData();

        int64 NotifiedGold = 0;
        int64 StoredGoldWhenNotified = 0;
        Data->OnGoldChanged.AddLambda([Data, &NotifiedGold, &StoredGoldWhenNotified](int64 Gold)
        {
            NotifiedGold = Gold;
            StoredGoldWhenNotified = Data->GetGold();
        });

        Data->HandleRewardResult(MakeReward(0, 1250));

        TestEqual(TEXT("골드: 사본"), Data->GetGold(), 1250LL);
        TestEqual(TEXT("골드: 알림"), NotifiedGold, 1250LL);
        TestEqual(TEXT("골드: 알릴 때 사본"), StoredGoldWhenNotified, 1250LL);
    }

    return true;
}

#endif
