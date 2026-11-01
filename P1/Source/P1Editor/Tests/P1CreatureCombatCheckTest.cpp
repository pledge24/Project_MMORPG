// 크리처 추출의 검사, 산출물 형식, 실제 몽타주에서 읽은 값을 고정한다.
//
// 왜 이것인가: 서버는 액션의 타격을 섹션 시작에서 잰 타격 시점에 판정하고, 타격 순번 i의 공격 범위를 i번째 타격 시점에 쓴다.
// 타격 노티파이 수가 기획 데이터의 타격 수와 다르거나, 기획 데이터의 액션에 에셋 행이 없으면 서버가 언제 무엇으로 판정할지 모른다.
// 콤보 입력 창이 섹션 끝을 넘으면 다음 단계가 시작된 뒤에도 앞 단계의 입력 창이 열려 있다. 추출은 이런 에셋을 0이 아닌 종료 코드로 막아야 한다.
// 루트 모션 곡선은 서버가 액션 중의 위치를 재는 데 쓰므로, 곡선의 끝이 섹션 전체의 이동량과 같아야 한다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Editor.CreatureCombat

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "CreatureCombat/P1CreatureCombatCheck.h"
#include "CreatureCombat/P1MontageExtract.h"
#include "GameFramework/Character.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1CreatureCombatCheckTest
{
    constexpr int32 MONSTER_ID = 5000;
    constexpr int32 WARRIOR_ID = 1;
    constexpr int32 COMBO_FIRST_ACTION_ID = 1001;
    constexpr int32 COMBO_LAST_ACTION_ID = 1002;
    constexpr int32 MONSTER_ACTION_ID = 2001;

    FP1CreatureCombatCheck::FCreature MakeCreature(int32 Id, const FString& Source, float Radius, float HalfHeight)
    {
        FP1CreatureCombatCheck::FCreature Creature;
        Creature.Id = Id;
        Creature.Source = Source;
        Creature.CapsuleRadius = Radius;
        Creature.CapsuleHalfHeight = HalfHeight;
        return Creature;
    }

    FP1CreatureCombatCheck::FAction MakeAction(int32 Id, const FString& Montage, const FString& Section, float Length, TArray<float> HitTimes)
    {
        FP1CreatureCombatCheck::FAction Action;
        Action.Id = Id;
        Action.Source = FString::Printf(TEXT("DT_Test 행 %d"), Id);
        Action.Montage = Montage;
        Action.Section = Section;
        Action.Length = Length;
        Action.HitTimes = MoveTemp(HitTimes);
        return Action;
    }

    /**
     * 몬스터 하나, 전사, 액션 셋. 전사 콤보 두 단계는 루트 모션이 있는 한 몽타주의 섹션이고 첫 단계만 콤보 입력 창이 있다.
     * 몬스터는 루트 모션이 없는 액션 하나를 쓴다. 표본은 1/30초 간격이다.
     */
    FP1CreatureCombatCheck::FInput MakeInput()
    {
        FP1CreatureCombatCheck::FInput Input;
        Input.DesignMonsterIds.Add(MONSTER_ID);
        Input.Monsters.Add(MakeCreature(MONSTER_ID, TEXT("DT_Test 행 5000"), 34.f, 88.f));
        Input.Classes.Add(MakeCreature(WARRIOR_ID, TEXT("BP_Test_C"), 42.f, 96.f));

        const FString WarriorMontage = TEXT("/Game/Test/AM_Warrior.AM_Warrior");
        FP1CreatureCombatCheck::FAction ComboFirst = MakeAction(COMBO_FIRST_ACTION_ID, WarriorMontage, TEXT("Combo1"), 0.05f, { 0.04f });
        ComboFirst.bHasRootMotion = true;
        ComboFirst.ComboWindows = { { 0.01f, 0.045f } };
        ComboFirst.RootMotion = { FVector::ZeroVector, FVector(10.004, 0.0, 0.0), FVector(14.5, -0.001, 0.0) };
        Input.Actions.Add(MoveTemp(ComboFirst));

        FP1CreatureCombatCheck::FAction ComboLast = MakeAction(COMBO_LAST_ACTION_ID, WarriorMontage, TEXT("Combo2"), 0.03f, { 0.02f });
        ComboLast.bHasRootMotion = true;
        ComboLast.RootMotion = { FVector::ZeroVector, FVector(5.0, 0.0, 0.0) };
        Input.Actions.Add(MoveTemp(ComboLast));

        Input.Actions.Add(MakeAction(MONSTER_ACTION_ID, TEXT("/Game/Test/AM_Monster.AM_Monster"), TEXT("Default"), 0.83333f, { 0.21f }));

        Input.DesignActions.Add(COMBO_FIRST_ACTION_ID, { 1, true, false });
        Input.DesignActions.Add(COMBO_LAST_ACTION_ID, { 1, false, false });
        Input.DesignActions.Add(MONSTER_ACTION_ID, { 1, false, true });
        return Input;
    }

    FP1CreatureCombatCheck::FAction& FindAction(FP1CreatureCombatCheck::FInput& Input, int32 ActionId)
    {
        return *Input.Actions.FindByPredicate([ActionId](const FP1CreatureCombatCheck::FAction& Action) { return Action.Id == ActionId; });
    }

    const TCHAR* EXPECTED_FILE =
        TEXT("{\"RootMotionSampleRate\":30,\n")
        TEXT("\"Monsters\":[\n")
        TEXT("{\"Id\":5000,\"CapsuleRadius\":34.0,\"CapsuleHalfHeight\":88.0}\n")
        TEXT("],\n")
        TEXT("\"Classes\":[\n")
        TEXT("{\"Id\":1,\"CapsuleRadius\":42.0,\"CapsuleHalfHeight\":96.0}\n")
        TEXT("],\n")
        TEXT("\"Actions\":[\n")
        TEXT("{\"Id\":1001,\"Montage\":\"/Game/Test/AM_Warrior.AM_Warrior\",\"Section\":\"Combo1\",\"Length\":0.05,\"HitTimes\":[0.04],")
        TEXT("\"ComboWindow\":[0.01,0.045],\"RootMotion\":[[0.0,0.0,0.0],[10.0,0.0,0.0],[14.5,0.0,0.0]]},\n")
        TEXT("{\"Id\":1002,\"Montage\":\"/Game/Test/AM_Warrior.AM_Warrior\",\"Section\":\"Combo2\",\"Length\":0.03,\"HitTimes\":[0.02],")
        TEXT("\"ComboWindow\":null,\"RootMotion\":[[0.0,0.0,0.0],[5.0,0.0,0.0]]},\n")
        TEXT("{\"Id\":2001,\"Montage\":\"/Game/Test/AM_Monster.AM_Monster\",\"Section\":\"Default\",\"Length\":0.8333,\"HitTimes\":[0.21],")
        TEXT("\"ComboWindow\":null,\"RootMotion\":[]}\n")
        TEXT("]}\n");

    /** 저장소의 실제 에셋. 값은 에디터에서 읽은 것이다(전사 콤보는 2026년 11월 1일, 근접 미니언은 2026년 10월 29일). */
    const TCHAR* WARRIOR_COMBO_MONTAGE = TEXT("/Game/P1/Characters/Player/Animations/AM_AttackA_Combo.AM_AttackA_Combo");
    const TCHAR* MELEE_MINION_MONTAGE = TEXT("/Game/P1/Characters/Monsters/MeleeMinion/Animations/Attack_C_SetB_Montage.Attack_C_SetB_Montage");
    const TCHAR* PLAYER_CLASS = TEXT("/Game/P1/Characters/Player/BP_Player.BP_Player_C");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1CreatureCombatCheckTest,
    "P1.Editor.CreatureCombat.Check",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1CreatureCombatCheckTest::RunTest(const FString& Parameters)
{
    using namespace P1CreatureCombatCheckTest;

    // 오류 하나만 있게 고친 입력을 검사하고, 0이 아닌 종료 코드와 메시지의 단서를 확인한다.
    auto ExpectRejected = [this](const FString& What, const FP1CreatureCombatCheck::FInput& Input, const TArray<FString>& Needles)
    {
        const FP1CreatureCombatCheck::FResult Result = FP1CreatureCombatCheck::Run(Input);
        TestNotEqual(What + TEXT(": 종료 코드"), Result.GetExitCode(), 0);
        TestTrue(What + TEXT(": 오류가 있으면 산출물을 만들지 않는다"), Result.File.IsEmpty());
        const FString Joined = FString::Join(Result.Errors, TEXT("\n"));
        for (const FString& Needle : Needles)
            TestTrue(FString::Printf(TEXT("%s: 오류에 '%s'가 있다. 오류: %s"), *What, *Needle, *Joined), Joined.Contains(Needle));
    };

    // 1) 올바른 입력은 통과하고, 크리처와 액션 하나가 한 줄인 파일을 만든다. 시간은 넷째 자리, 거리는 둘째 자리에서 반올림한다.
    {
        const FP1CreatureCombatCheck::FResult Result = FP1CreatureCombatCheck::Run(MakeInput());
        TestEqual(TEXT("올바른 입력의 종료 코드"), Result.GetExitCode(), 0);
        TestEqual(TEXT("오류 없음"), FString::Join(Result.Errors, TEXT("\n")), FString());
        TestEqual(TEXT("산출물 형식"), Result.File, FString(EXPECTED_FILE));
    }

    // 2) 몬스터 표의 몬스터에 에셋 테이블의 행이 없다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.DesignMonsterIds.Add(5001);
        ExpectRejected(TEXT("짝이 없는 몬스터"), Input, { TEXT("몬스터 5001"), TEXT("행이 없다") });
    }

    // 3) 에셋 테이블에만 있는 몬스터.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.DesignMonsterIds.Reset();
        ExpectRejected(TEXT("몬스터 표에 없는 몬스터"), Input, { TEXT("몬스터 5000"), TEXT("몬스터 표에 없다") });
    }

    // 4) 기획 데이터의 액션에 액션 에셋 테이블의 행이 없다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.DesignActions.Add(2002, { 1, false, true });
        ExpectRejected(TEXT("짝이 없는 액션"), Input, { TEXT("액션 2002"), TEXT("액션 에셋 테이블에 행이 없다") });
    }

    // 5) 액션 에셋 테이블에만 있는 액션.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.DesignActions.Remove(MONSTER_ACTION_ID);
        ExpectRejected(TEXT("액션 표에 없는 액션"), Input, { TEXT("액션 2001"), TEXT("액션 표에 없다") });
    }

    // 6) 섹션 안의 타격 노티파이 수가 기획 데이터의 타격 수와 다르다. 없을 때와 많을 때.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, MONSTER_ACTION_ID).HitTimes.Reset();
        ExpectRejected(TEXT("타격 노티파이 없음"), Input, { TEXT("액션 2001"), TEXT("Default"), TEXT("타격 노티파이(P1 Attack Hit)가 0개"), TEXT("타격은 1개") });

        Input = MakeInput();
        FindAction(Input, MONSTER_ACTION_ID).HitTimes = { 0.21f, 0.5f };
        ExpectRejected(TEXT("타격 노티파이가 많음"), Input, { TEXT("액션 2001"), TEXT("2개"), TEXT("타격은 1개") });
    }

    // 7) 섹션 밖의 타격 시점.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, MONSTER_ACTION_ID).HitTimes = { 0.9f };
        ExpectRejected(TEXT("섹션 밖의 타격"), Input, { TEXT("0.9초"), TEXT("섹션 밖") });
    }

    // 8) 다음 액션이 있는데 콤보 입력 창이 없거나 둘이다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, COMBO_FIRST_ACTION_ID).ComboWindows.Reset();
        ExpectRejected(TEXT("콤보 입력 창 없음"), Input, { TEXT("액션 1001"), TEXT("Combo1"), TEXT("콤보 입력 창(P1 Combo Window)이 0개") });

        Input = MakeInput();
        FindAction(Input, COMBO_FIRST_ACTION_ID).ComboWindows.Add({ 0.02f, 0.03f });
        ExpectRejected(TEXT("콤보 입력 창 둘"), Input, { TEXT("액션 1001"), TEXT("콤보 입력 창(P1 Combo Window)이 2개") });
    }

    // 9) 콤보 입력 창이 섹션 끝을 넘거나 섹션 끝에서 끝난다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, COMBO_FIRST_ACTION_ID).ComboWindows = { { 0.01f, 0.06f } };
        ExpectRejected(TEXT("섹션 끝을 넘는 입력 창"), Input, { TEXT("액션 1001"), TEXT("0.06초"), TEXT("섹션 끝") });

        Input = MakeInput();
        FindAction(Input, COMBO_FIRST_ACTION_ID).ComboWindows = { { 0.01f, 0.05f } };
        ExpectRejected(TEXT("섹션 끝에서 끝나는 입력 창"), Input, { TEXT("액션 1001"), TEXT("0.05초"), TEXT("섹션 끝") });
    }

    // 10) 다음 액션이 없는데 콤보 입력 창이 있다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, COMBO_LAST_ACTION_ID).ComboWindows = { { 0.0f, 0.02f } };
        ExpectRejected(TEXT("마지막 단계의 입력 창"), Input, { TEXT("액션 1002"), TEXT("다음 액션이 없는데") });
    }

    // 11) 몬스터가 쓰는 액션의 루트 모션. 서버가 몬스터를 옮기지 않으므로 받지 않는다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, MONSTER_ACTION_ID).bHasRootMotion = true;
        ExpectRejected(TEXT("몬스터 액션의 루트 모션"), Input, { TEXT("액션 2001"), TEXT("루트 모션") });
    }

    // 12) 빈 몽타주 칸.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, MONSTER_ACTION_ID).Montage.Reset();
        ExpectRejected(TEXT("빈 몽타주 칸"), Input, { TEXT("액션 2001"), TEXT("몽타주 칸이 비었다") });
    }

    // 13) 캡슐 크기.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Classes[0].CapsuleRadius = 0.f;
        ExpectRejected(TEXT("캡슐 반지름 0"), Input, { TEXT("직업 1"), TEXT("캡슐") });
    }

    // 14) 직업이 하나도 없다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Classes.Reset();
        ExpectRejected(TEXT("직업 없음"), Input, { TEXT("CharacterClassPlayers") });
    }

    // 15) 재생 속도가 1이 아닌 몽타주. 추출한 시간이 실제로 재생되는 초와 달라진다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        FindAction(Input, COMBO_FIRST_ACTION_ID).RateScale = 1.5f;
        ExpectRejected(TEXT("재생 속도"), Input, { TEXT("액션 1001"), TEXT("RateScale") });
    }

    // 16) 서버 기획 데이터 JSON. 다음 액션과 몬스터의 일반 공격은 서버만 받는 열이다.
    {
        const FString ActionJson = TEXT("[{\"Id\":1001,\"NextAction\":1002,\"Hits\":[{\"HitIndex\":0},{\"HitIndex\":1}]},")
            TEXT("{\"Id\":1002,\"NextAction\":null,\"Hits\":[{\"HitIndex\":0}]},{\"Id\":2001,\"NextAction\":null,\"Hits\":[{\"HitIndex\":0}]}]");
        const FString MonsterJson = TEXT("[{\"Id\":5000,\"NormalAttackAction\":2001},{\"Id\":5003,\"NormalAttackAction\":2001}]");
        FP1CreatureCombatCheck::FInput Input;
        TArray<FString> Errors;
        TestTrue(TEXT("기획 데이터 읽기"), FP1CreatureCombatCheck::ParseDesignData(ActionJson, MonsterJson, Input, Errors));
        TestEqual(TEXT("오류 없음"), FString::Join(Errors, TEXT("\n")), FString());
        TestEqual(TEXT("몬스터 번호 수"), Input.DesignMonsterIds.Num(), 2);
        if (TestEqual(TEXT("액션 수"), Input.DesignActions.Num(), 3))
        {
            TestEqual(TEXT("1001의 타격 수"), Input.DesignActions[1001].HitCount, 2);
            TestTrue(TEXT("1001은 다음 액션이 있다"), Input.DesignActions[1001].bHasNextAction);
            TestFalse(TEXT("1002는 다음 액션이 없다"), Input.DesignActions[1002].bHasNextAction);
            TestFalse(TEXT("1001은 몬스터가 쓰지 않는다"), Input.DesignActions[1001].bUsedByMonster);
            TestTrue(TEXT("2001은 몬스터가 쓴다"), Input.DesignActions[2001].bUsedByMonster);
        }

        FP1CreatureCombatCheck::FInput Broken;
        TestFalse(TEXT("Hits가 없는 액션"), FP1CreatureCombatCheck::ParseDesignData(TEXT("[{\"Id\":1001,\"NextAction\":null}]"), MonsterJson, Broken, Errors));
        TestFalse(TEXT("Id가 없는 몬스터"), FP1CreatureCombatCheck::ParseDesignData(ActionJson, TEXT("[{\"Name\":\"x\"}]"), Broken, Errors));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1CreatureCombatExtractTest,
    "P1.Editor.CreatureCombat.Extract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1CreatureCombatExtractTest::RunTest(const FString& Parameters)
{
    using namespace P1CreatureCombatCheckTest;

    const UAnimMontage* WarriorCombo = LoadObject<UAnimMontage>(nullptr, WARRIOR_COMBO_MONTAGE);
    const UClass* PlayerClass = LoadObject<UClass>(nullptr, PLAYER_CLASS);
    const UAnimMontage* MeleeMinion = LoadObject<UAnimMontage>(nullptr, MELEE_MINION_MONTAGE);
    if (TestNotNull(TEXT("전사 콤보 몽타주"), WarriorCombo) == false || TestNotNull(TEXT("플레이어 블루프린트"), PlayerClass) == false
        || TestNotNull(TEXT("근접 미니언 몽타주"), MeleeMinion) == false)
        return false;

    // 1) 전사 콤보 두 번째 섹션의 길이, 타격 시점, 콤보 입력 창이 에디터 값과 같다. 시간은 섹션 시작(1.8333초)에서 잰다.
    //    루트 모션 곡선의 끝은 섹션 전체의 루트 모션 이동량과 같다. 곡선은 구간마다 꺼내 쌓았고, 비교 값은 한 번에 꺼낸 것이다.
    {
        const FTransform MeshToActor = PlayerClass->GetDefaultObject<ACharacter>()->GetMesh()->GetRelativeTransform();
        FP1CreatureCombatCheck::FAction Action;
        if (TestTrue(TEXT("Combo2 섹션을 찾는다"), P1MontageExtract::ExtractSection(*WarriorCombo, TEXT("Combo2"), MeshToActor, Action)) == false)
            return false;

        TestTrue(TEXT("전사 콤보 몽타주는 루트 모션이 있다"), Action.bHasRootMotion);
        TestEqual(TEXT("Combo2의 길이(에디터의 섹션 경계 1.8333초~3.2초)"), Action.Length, 1.3666667f, 0.0001f);
        TestEqual(TEXT("표본 수는 1/30초 간격에 끝 하나"), Action.RootMotion.Num(), FMath::CeilToInt32(Action.Length * 30.f - KINDA_SMALL_NUMBER) + 1);
        if (TestEqual(TEXT("타격 노티파이 수"), Action.HitTimes.Num(), 1))
            TestEqual(TEXT("Combo2의 타격 시점(에디터 값 2.6133초)"), Action.HitTimes[0], 2.6133f - 1.8333333f, 0.001f);
        if (TestEqual(TEXT("콤보 입력 창 수"), Action.ComboWindows.Num(), 1))
        {
            TestEqual(TEXT("Combo2 입력 창의 시작(에디터 값 2.5009초)"), Action.ComboWindows[0].Start, 2.5009f - 1.8333333f, 0.001f);
            TestEqual(TEXT("Combo2 입력 창의 끝(에디터 값 3.1667초)"), Action.ComboWindows[0].End, 3.1667f - 1.8333333f, 0.001f);
        }

        const int32 SectionIndex = WarriorCombo->GetSectionIndex(TEXT("Combo2"));
        float Start = 0.f;
        float End = 0.f;
        WarriorCombo->GetSectionStartAndEndTime(SectionIndex, Start, End);
        const FVector Whole = MeshToActor.TransformVector(WarriorCombo->ExtractRootMotionFromTrackRange(Start, End, FAnimExtractContext()).GetTranslation());
        if (Action.RootMotion.IsEmpty() == false)
        {
            TestTrue(FString::Printf(TEXT("곡선의 끝 %s가 전체 이동량 %s와 같다"), *Action.RootMotion.Last().ToString(), *Whole.ToString()),
                Action.RootMotion.Last().Equals(Whole, 0.1));
            TestTrue(TEXT("곡선의 처음은 0"), Action.RootMotion[0].IsZero());
        }
        // 메시가 yaw -90도로 붙어 있어 메시의 +Y 이동이 캐릭터의 앞(+X)이 된다.
        TestTrue(FString::Printf(TEXT("전사 2단은 앞으로 나간다: %s"), *Whole.ToString()), Whole.X > 100.0 && FMath::Abs(Whole.Y) < Whole.X);
    }

    // 2) 마지막 섹션에는 콤보 입력 창이 없고, 없는 섹션은 찾지 못한다.
    {
        FP1CreatureCombatCheck::FAction Action;
        if (TestTrue(TEXT("Combo4 섹션을 찾는다"), P1MontageExtract::ExtractSection(*WarriorCombo, TEXT("Combo4"), FTransform::Identity, Action)))
            TestEqual(TEXT("Combo4의 콤보 입력 창 수"), Action.ComboWindows.Num(), 0);
        TestFalse(TEXT("없는 섹션"), P1MontageExtract::ExtractSection(*WarriorCombo, TEXT("Combo9"), FTransform::Identity, Action));
    }

    // 3) 근접 미니언은 루트 모션이 없고, 타격 시점은 몽타주에 둔 노티파이의 시점이다.
    {
        FP1CreatureCombatCheck::FAction Action;
        if (TestTrue(TEXT("근접 미니언의 Default 섹션을 찾는다"), P1MontageExtract::ExtractSection(*MeleeMinion, TEXT("Default"), FTransform::Identity, Action)) == false)
            return false;

        TestFalse(TEXT("근접 미니언은 루트 모션이 없다"), Action.bHasRootMotion);
        TestTrue(TEXT("루트 모션이 없으면 곡선이 없다"), Action.RootMotion.IsEmpty());
        TestEqual(TEXT("근접 미니언 몽타주의 길이(에디터 값 0.8333초)"), Action.Length, 0.8333333f, 0.0001f);
        if (TestEqual(TEXT("타격 노티파이 수"), Action.HitTimes.Num(), 1))
            TestEqual(TEXT("근접 미니언의 타격 시점(에디터 값 0.21초)"), Action.HitTimes[0], 0.21f, 0.001f);
    }

    return true;
}

#endif
