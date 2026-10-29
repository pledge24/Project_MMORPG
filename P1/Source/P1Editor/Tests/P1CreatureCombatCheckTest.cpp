// 크리처 추출의 검사, 산출물 형식, 실제 몽타주에서 읽은 값을 고정한다.
//
// 왜 이것인가: 서버는 공격을 알린 뒤 피격을 판정하기까지의 시간을 상수로 두었고, 애니메이션이 바뀌어도 그 상수는 따라오지 않았다.
// 몽타주에서 타격 시점을 추출하면 맞출 일은 없지만, 타격 노티파이가 빠진 몽타주나 몬스터 표와 짝이 없는 몬스터를 그대로 넘기면
// 서버가 피격을 언제 판정할지 모른다. 추출은 이런 에셋을 0이 아닌 종료 코드로 막아야 한다.
// 루트 모션 곡선은 서버가 공격 중의 위치를 재는 데 쓰므로, 곡선의 끝이 몽타주 전체의 이동량과 같아야 한다.
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

    FP1CreatureCombatCheck::FAttack MakeAttack(const FString& Montage, float Length, TArray<float> HitTimes)
    {
        FP1CreatureCombatCheck::FAttack Attack;
        Attack.Montage = Montage;
        FP1CreatureCombatCheck::FSection& Section = Attack.Sections.AddDefaulted_GetRef();
        Section.Name = TEXT("Default");
        Section.Length = Length;
        Section.HitTimes = MoveTemp(HitTimes);
        return Attack;
    }

    FP1CreatureCombatCheck::FCreature MakeCreature(int32 Id, const FString& Source, FP1CreatureCombatCheck::FAttack Attack)
    {
        FP1CreatureCombatCheck::FCreature Creature;
        Creature.Id = Id;
        Creature.Source = Source;
        Creature.CapsuleRadius = 34.f;
        Creature.CapsuleHalfHeight = 88.f;
        Creature.Attacks.Add(MoveTemp(Attack));
        return Creature;
    }

    /** 몬스터 하나와 전사. 전사의 공격만 루트 모션이 있고, 표본은 1/30초 간격이다. */
    FP1CreatureCombatCheck::FInput MakeInput()
    {
        FP1CreatureCombatCheck::FInput Input;
        Input.DesignMonsterIds.Add(MONSTER_ID);
        Input.Monsters.Add(MakeCreature(MONSTER_ID, TEXT("DT_Test 행 5000"), MakeAttack(TEXT("/Game/Test/AM_Monster.AM_Monster"), 0.83333f, { 0.21f })));

        FP1CreatureCombatCheck::FAttack WarriorAttack = MakeAttack(TEXT("/Game/Test/AM_Warrior.AM_Warrior"), 0.05f, { 0.04f });
        WarriorAttack.bHasRootMotion = true;
        WarriorAttack.Sections[0].RootMotion = { FVector::ZeroVector, FVector(10.004, 0.0, 0.0), FVector(14.5, -0.001, 0.0) };
        FP1CreatureCombatCheck::FCreature Warrior = MakeCreature(WARRIOR_ID, TEXT("BP_Test_C"), MoveTemp(WarriorAttack));
        Warrior.CapsuleRadius = 42.f;
        Warrior.CapsuleHalfHeight = 96.f;
        Input.Classes.Add(MoveTemp(Warrior));
        return Input;
    }

    const TCHAR* EXPECTED_FILE =
        TEXT("{\"RootMotionSampleRate\":30,\n")
        TEXT("\"Monsters\":[\n")
        TEXT("{\"Id\":5000,\"CapsuleRadius\":34.0,\"CapsuleHalfHeight\":88.0,\"Attacks\":[{\"Montage\":\"/Game/Test/AM_Monster.AM_Monster\",")
        TEXT("\"Sections\":[{\"Name\":\"Default\",\"Length\":0.8333,\"HitTimes\":[0.21],\"RootMotion\":[]}]}]}\n")
        TEXT("],\n")
        TEXT("\"Classes\":[\n")
        TEXT("{\"Id\":1,\"CapsuleRadius\":42.0,\"CapsuleHalfHeight\":96.0,\"Attacks\":[{\"Montage\":\"/Game/Test/AM_Warrior.AM_Warrior\",")
        TEXT("\"Sections\":[{\"Name\":\"Default\",\"Length\":0.05,\"HitTimes\":[0.04],\"RootMotion\":[[0.0,0.0,0.0],[10.0,0.0,0.0],[14.5,0.0,0.0]]}]}]}\n")
        TEXT("]}\n");

    /** 저장소의 실제 에셋. 값은 2026년 10월 29일 에디터에서 읽은 것이다. */
    const TCHAR* WARRIOR_MONTAGE = TEXT("/Game/P1/Characters/Player/Animations/AM_AttackA1.AM_AttackA1");
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

    // 1) 올바른 입력은 통과하고, 크리처 하나가 한 줄인 파일을 만든다. 시간은 넷째 자리, 거리는 둘째 자리에서 반올림한다.
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

    // 4) 타격 노티파이가 없는 섹션.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Monsters[0].Attacks[0].Sections[0].HitTimes.Reset();
        ExpectRejected(TEXT("타격 노티파이 없음"), Input, { TEXT("AM_Monster"), TEXT("Default"), TEXT("타격 노티파이") });
    }

    // 5) 섹션 밖의 타격 시점.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Monsters[0].Attacks[0].Sections[0].HitTimes = { 0.9f };
        ExpectRejected(TEXT("섹션 밖의 타격"), Input, { TEXT("0.9초"), TEXT("섹션 밖") });
    }

    // 6) 몬스터의 루트 모션. 서버가 몬스터를 옮기지 않으므로 받지 않는다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Monsters[0].Attacks[0].bHasRootMotion = true;
        ExpectRejected(TEXT("몬스터의 루트 모션"), Input, { TEXT("몬스터 5000"), TEXT("루트 모션") });
    }

    // 7) 빈 몽타주 칸과 공격 몽타주가 없는 크리처.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Monsters[0].Attacks.AddDefaulted();
        ExpectRejected(TEXT("빈 몽타주 칸"), Input, { TEXT("몬스터 5000"), TEXT("2번째 칸") });

        Input = MakeInput();
        Input.Classes[0].Attacks.Reset();
        ExpectRejected(TEXT("공격 몽타주 없음"), Input, { TEXT("직업 1"), TEXT("공격 몽타주가 없다") });
    }

    // 8) 캡슐 크기.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Classes[0].CapsuleRadius = 0.f;
        ExpectRejected(TEXT("캡슐 반지름 0"), Input, { TEXT("직업 1"), TEXT("캡슐") });
    }

    // 9) 직업이 하나도 없다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Classes.Reset();
        ExpectRejected(TEXT("직업 없음"), Input, { TEXT("CharacterClassPlayers") });
    }

    // 10) 몬스터 표 JSON의 번호.
    {
        TSet<int32> Ids;
        TArray<FString> Errors;
        TestTrue(TEXT("몬스터 번호 읽기"), FP1CreatureCombatCheck::ParseMonsterIds(TEXT("[{\"Id\":5000},{\"Id\":5001}]"), Ids, Errors));
        TestEqual(TEXT("몬스터 번호 수"), Ids.Num(), 2);
        TestFalse(TEXT("Id가 없는 행"), FP1CreatureCombatCheck::ParseMonsterIds(TEXT("[{\"Name\":\"x\"}]"), Ids, Errors));
    }

    // 11) 재생 속도가 1이 아닌 몽타주. 추출한 시간이 실제로 재생되는 초와 달라진다.
    {
        FP1CreatureCombatCheck::FInput Input = MakeInput();
        Input.Classes[0].Attacks[0].RateScale = 1.5f;
        ExpectRejected(TEXT("재생 속도"), Input, { TEXT("직업 1"), TEXT("RateScale") });
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

    const UAnimMontage* Warrior = LoadObject<UAnimMontage>(nullptr, WARRIOR_MONTAGE);
    const UClass* PlayerClass = LoadObject<UClass>(nullptr, PLAYER_CLASS);
    const UAnimMontage* MeleeMinion = LoadObject<UAnimMontage>(nullptr, MELEE_MINION_MONTAGE);
    if (TestNotNull(TEXT("전사 몽타주"), Warrior) == false || TestNotNull(TEXT("플레이어 블루프린트"), PlayerClass) == false
        || TestNotNull(TEXT("근접 미니언 몽타주"), MeleeMinion) == false)
        return false;

    // 1) 루트 모션 곡선의 끝이 몽타주 전체의 루트 모션 이동량과 같다. 곡선은 구간마다 꺼내 쌓았고, 비교 값은 한 번에 꺼낸 것이다.
    {
        const FTransform MeshToActor = PlayerClass->GetDefaultObject<ACharacter>()->GetMesh()->GetRelativeTransform();
        const FP1CreatureCombatCheck::FAttack Attack = P1MontageExtract::Extract(*Warrior, MeshToActor);
        if (TestEqual(TEXT("전사 몽타주의 섹션 수"), Attack.Sections.Num(), 1) == false)
            return false;

        const FP1CreatureCombatCheck::FSection& Section = Attack.Sections[0];
        TestTrue(TEXT("전사 몽타주는 루트 모션이 있다"), Attack.bHasRootMotion);
        TestEqual(TEXT("전사 몽타주의 길이(에디터 값 1.8333초)"), Section.Length, 1.8333334f, 0.0001f);
        TestEqual(TEXT("표본 수는 1/30초 간격에 끝 하나"), Section.RootMotion.Num(), FMath::CeilToInt32(Section.Length * 30.f - KINDA_SMALL_NUMBER) + 1);
        if (TestEqual(TEXT("타격 노티파이 수"), Section.HitTimes.Num(), 1))
            TestEqual(TEXT("전사 첫 타격의 타격 시점(에디터 값 1.09초)"), Section.HitTimes[0], 1.09f, 0.001f);

        float Start = 0.f;
        float End = 0.f;
        Warrior->GetSectionStartAndEndTime(0, Start, End);
        const FVector Whole = MeshToActor.TransformVector(Warrior->ExtractRootMotionFromTrackRange(Start, End, FAnimExtractContext()).GetTranslation());
        if (Section.RootMotion.IsEmpty() == false)
        {
            TestTrue(FString::Printf(TEXT("곡선의 끝 %s가 전체 이동량 %s와 같다"), *Section.RootMotion.Last().ToString(), *Whole.ToString()),
                Section.RootMotion.Last().Equals(Whole, 0.1));
            TestTrue(TEXT("곡선의 처음은 0"), Section.RootMotion[0].IsZero());
        }
        // 메시가 yaw -90도로 붙어 있어 메시의 +Y 이동이 캐릭터의 앞(+X)이 된다.
        TestTrue(FString::Printf(TEXT("전사 첫 타격은 앞으로 나간다: %s"), *Whole.ToString()), Whole.X > 100.0 && FMath::Abs(Whole.Y) < Whole.X);
    }

    // 2) 근접 미니언은 루트 모션이 없고, 타격 시점은 몽타주에 둔 노티파이의 시점이다.
    {
        const FP1CreatureCombatCheck::FAttack Attack = P1MontageExtract::Extract(*MeleeMinion, FTransform::Identity);
        if (TestEqual(TEXT("근접 미니언 몽타주의 섹션 수"), Attack.Sections.Num(), 1) == false)
            return false;

        const FP1CreatureCombatCheck::FSection& Section = Attack.Sections[0];
        TestFalse(TEXT("근접 미니언은 루트 모션이 없다"), Attack.bHasRootMotion);
        TestTrue(TEXT("루트 모션이 없으면 곡선이 없다"), Section.RootMotion.IsEmpty());
        TestEqual(TEXT("근접 미니언 몽타주의 길이(에디터 값 0.8333초)"), Section.Length, 0.8333333f, 0.0001f);
        if (TestEqual(TEXT("타격 노티파이 수"), Section.HitTimes.Num(), 1))
            TestEqual(TEXT("근접 미니언의 타격 시점(에디터 값 0.21초)"), Section.HitTimes[0], 0.21f, 0.001f);
    }

    return true;
}

#endif
