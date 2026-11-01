// 시퀀스를 몽타주에 이어 붙여 섹션을 만드는 에디터 함수와, 그 함수로 만든 전사 콤보 몽타주를 고정한다.
//
// 왜 이것인가: 전사 콤보는 단계마다 회복 동작이 있는 시퀀스 넷을 몽타주 하나에 이어 붙인 것이다. 섹션 경계가 시퀀스 경계와 어긋나거나
// 섹션끼리 이어져 있으면, 입력이 없어도 다음 단계가 재생되거나 단계의 타격 시점이 다른 섹션에 들어간다.
// 서버는 섹션마다 타격 시점과 입력 창 구간을 몽타주에서 가져가야 하므로, 섹션마다 노티파이가 빠지지 않아야 한다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Editor.MontageSections

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/P1MontageSectionLibrary.h"
#include "Game/Combat/P1AnimNotify_AttackHit.h"
#include "Game/Combat/P1AnimNotifyState_ComboWindow.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1MontageSectionLibraryTest
{
    const TCHAR* const SEQUENCES[] = {
        TEXT("/Game/P1/Characters/Player/Animations/A_AttackA1.A_AttackA1"),
        TEXT("/Game/P1/Characters/Player/Animations/A_AttackA2.A_AttackA2"),
        TEXT("/Game/P1/Characters/Player/Animations/A_AttackA3.A_AttackA3"),
        TEXT("/Game/P1/Characters/Player/Animations/A_AttackA4.A_AttackA4"),
    };
    const TCHAR* const COMBO_SECTION_NAMES[] = { TEXT("Combo1"), TEXT("Combo2"), TEXT("Combo3"), TEXT("Combo4") };
    constexpr int32 COMBO_STEP_COUNT = UE_ARRAY_COUNT(SEQUENCES);
    static_assert(UE_ARRAY_COUNT(COMBO_SECTION_NAMES) == COMBO_STEP_COUNT);

    const TCHAR* const COMBO_MONTAGE = TEXT("/Game/P1/Characters/Player/Animations/AM_AttackA_Combo.AM_AttackA_Combo");

    /** 전사 콤보 몽타주의 타격 시점과 입력 창 구간(몽타주 시작 기준 초). 에디터에서 맞춘 값이고, 마지막 단계에는 입력 창이 없다. */
    const float COMBO_HIT_TIMES[] = { 0.842387f, 2.613333f, 3.41f, 5.018222f };
    const float COMBO_WINDOW_STARTS[] = { 0.950978f, 2.841457f, 3.497609f };
    const float COMBO_WINDOW_ENDS[] = { 1.066667f, 3.166667f, 3.966667f };
    static_assert(UE_ARRAY_COUNT(COMBO_HIT_TIMES) == COMBO_STEP_COUNT);
    static_assert(UE_ARRAY_COUNT(COMBO_WINDOW_STARTS) == COMBO_STEP_COUNT - 1);
    static_assert(UE_ARRAY_COUNT(COMBO_WINDOW_ENDS) == COMBO_STEP_COUNT - 1);

    constexpr float TIME_TOLERANCE = 1.e-4f;

    TArray<UAnimSequenceBase*> LoadSequences()
    {
        TArray<UAnimSequenceBase*> Sequences;
        for (const TCHAR* Path : SEQUENCES)
            Sequences.Add(LoadObject<UAnimSequenceBase>(nullptr, Path));
        return Sequences;
    }

    TArray<FName> ComboSectionNames()
    {
        TArray<FName> Names;
        for (const TCHAR* Name : COMBO_SECTION_NAMES)
            Names.Add(Name);
        return Names;
    }

    UAnimMontage* MakeMontage(const UAnimSequenceBase& Sequence)
    {
        UAnimMontage* Montage = NewObject<UAnimMontage>(GetTransientPackage());
        Montage->SetSkeleton(Sequence.GetSkeleton());
        return Montage;
    }

    /** 섹션이 시퀀스마다 하나이고, 이름과 경계(시퀀스 길이의 누적)가 맞으며, 섹션끼리 잇지 않았는지 검사한다. */
    void TestSectionsFollowSequences(FAutomationTestBase& Test, const UAnimMontage& Montage, const TArray<UAnimSequenceBase*>& Sequences)
    {
        if (Test.TestEqual(TEXT("섹션 수"), Montage.CompositeSections.Num(), Sequences.Num()) == false)
            return;

        const TArray<FName> Names = ComboSectionNames();
        float Expected = 0.f;
        for (int32 Index = 0; Index < Sequences.Num(); ++Index)
        {
            const FString What = FString::Printf(TEXT("%d번째 섹션"), Index + 1);
            float Start = 0.f;
            float End = 0.f;
            Montage.GetSectionStartAndEndTime(Index, Start, End);
            Test.TestEqual(What + TEXT(" 이름"), Montage.GetSectionName(Index), Names[Index]);
            Test.TestEqual(What + TEXT(" 시작"), Start, Expected, TIME_TOLERANCE);
            Expected += Sequences[Index]->GetPlayLength();
            Test.TestEqual(What + TEXT(" 끝"), End, Expected, TIME_TOLERANCE);

            // 다음 섹션은 클라이언트가 입력을 받았을 때만 지정한다.
            Test.TestEqual(What + TEXT("의 다음 섹션"), Montage.CompositeSections[Index].NextSectionName, FName(NAME_None));
        }
        Test.TestEqual(TEXT("몽타주 길이"), Montage.GetPlayLength(), Expected, TIME_TOLERANCE);
    }

    /** 섹션 [Start, End)에서 시작하는 T 타입 노티파이나 노티파이 스테이트. 마지막 섹션만 끝을 포함한다. */
    template <typename T>
    TArray<const FAnimNotifyEvent*> NotifiesInSection(const UAnimMontage& Montage, int32 SectionIndex)
    {
        float Start = 0.f;
        float End = 0.f;
        Montage.GetSectionStartAndEndTime(SectionIndex, Start, End);
        const bool bLastSection = SectionIndex == Montage.CompositeSections.Num() - 1;

        TArray<const FAnimNotifyEvent*> Found;
        for (const FAnimNotifyEvent& Notify : Montage.Notifies)
        {
            const bool bIsT = Cast<T>(Notify.Notify) || Cast<T>(Notify.NotifyStateClass);
            const float Time = Notify.GetTriggerTime();
            if (bIsT && Time >= Start && (Time < End || (bLastSection && Time <= End)))
                Found.Add(&Notify);
        }
        return Found;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MontageSectionLibraryTest,
    "P1.Editor.MontageSections.SetSequenceSections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MontageSectionLibraryTest::RunTest(const FString& Parameters)
{
    using namespace P1MontageSectionLibraryTest;

    const TArray<UAnimSequenceBase*> Sequences = LoadSequences();
    for (const UAnimSequenceBase* Sequence : Sequences)
    {
        if (TestNotNull(TEXT("전사 공격 시퀀스"), Sequence) == false)
            return false;
    }

    // 시퀀스 넷이 한 슬롯 트랙에 차례로 이어지고, 시퀀스마다 그 시작에 섹션이 하나다.
    {
        UAnimMontage* Montage = MakeMontage(*Sequences[0]);
        TestTrue(TEXT("성공"), UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences, ComboSectionNames()));

        TestEqual(TEXT("슬롯 트랙 하나"), Montage->SlotAnimTracks.Num(), 1);
        const TArray<FAnimSegment>& Segments = Montage->SlotAnimTracks[0].AnimTrack.AnimSegments;
        if (TestEqual(TEXT("세그먼트 수"), Segments.Num(), Sequences.Num()))
        {
            float Expected = 0.f;
            for (int32 Index = 0; Index < Sequences.Num(); ++Index)
            {
                const FString What = FString::Printf(TEXT("%d번째 세그먼트"), Index + 1);
                TestTrue(What + TEXT("의 시퀀스"), Segments[Index].GetAnimReference() == Sequences[Index]);
                TestEqual(What + TEXT(" 시작"), Segments[Index].StartPos, Expected, TIME_TOLERANCE);
                Expected += Sequences[Index]->GetPlayLength();
            }
        }
        TestSectionsFollowSequences(*this, *Montage, Sequences);
    }

    // 있던 세그먼트와 섹션은 남지 않는다.
    {
        UAnimMontage* Montage = MakeMontage(*Sequences[0]);
        UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences, ComboSectionNames());
        TestTrue(TEXT("다시 만들기 성공"), UP1MontageSectionLibrary::SetSequenceSections(Montage, { Sequences[2] }, { TEXT("Only") }));
        TestEqual(TEXT("다시 만든 세그먼트 수"), Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num(), 1);
        TestEqual(TEXT("다시 만든 섹션 수"), Montage->CompositeSections.Num(), 1);
        TestEqual(TEXT("다시 만든 몽타주 길이"), Montage->GetPlayLength(), Sequences[2]->GetPlayLength(), TIME_TOLERANCE);
    }

    // 잘못된 입력은 몽타주를 바꾸지 않는다.
    {
        UAnimMontage* Montage = MakeMontage(*Sequences[0]);
        const int32 SectionsBefore = Montage->CompositeSections.Num();
        TArray<UAnimSequenceBase*> WithEmpty = Sequences;
        WithEmpty[1] = nullptr;

        TestFalse(TEXT("빈 몽타주"), UP1MontageSectionLibrary::SetSequenceSections(nullptr, Sequences, ComboSectionNames()));
        TestFalse(TEXT("시퀀스 없음"), UP1MontageSectionLibrary::SetSequenceSections(Montage, {}, {}));
        TestFalse(TEXT("개수 불일치"), UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences, { TEXT("Combo1") }));
        TestFalse(TEXT("빈 칸"), UP1MontageSectionLibrary::SetSequenceSections(Montage, WithEmpty, ComboSectionNames()));
        TestFalse(TEXT("겹친 이름"), UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences,
            { TEXT("Combo1"), TEXT("Combo1"), TEXT("Combo3"), TEXT("Combo4") }));
        TestFalse(TEXT("없는 이름"), UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences,
            { TEXT("Combo1"), NAME_None, TEXT("Combo3"), TEXT("Combo4") }));

        UAnimMontage* OtherSkeleton = NewObject<UAnimMontage>(GetTransientPackage());
        TestFalse(TEXT("다른 스켈레톤"), UP1MontageSectionLibrary::SetSequenceSections(OtherSkeleton, Sequences, ComboSectionNames()));

        TestEqual(TEXT("바뀌지 않은 섹션 수"), Montage->CompositeSections.Num(), SectionsBefore);
        TestEqual(TEXT("바뀌지 않은 세그먼트 수"), Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num(), 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1MontageUnlinkSectionsTest,
    "P1.Editor.MontageSections.UnlinkSections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1MontageUnlinkSectionsTest::RunTest(const FString& Parameters)
{
    using namespace P1MontageSectionLibraryTest;

    const TArray<UAnimSequenceBase*> Sequences = LoadSequences();
    if (TestNotNull(TEXT("전사 공격 시퀀스"), Sequences[0]) == false)
        return false;

    // 에디터의 섹션 패널에서 이은 것처럼 섹션을 차례로 이어 두고 끊는다.
    UAnimMontage* Montage = MakeMontage(*Sequences[0]);
    UP1MontageSectionLibrary::SetSequenceSections(Montage, Sequences, ComboSectionNames());
    const TArray<FName> Names = ComboSectionNames();
    for (int32 Index = 0; Index + 1 < Names.Num(); ++Index)
        Montage->CompositeSections[Index].NextSectionName = Names[Index + 1];

    TestEqual(TEXT("끊은 연결 수"), UP1MontageSectionLibrary::UnlinkSections(Montage), Names.Num() - 1);
    TestSectionsFollowSequences(*this, *Montage, Sequences);
    TestEqual(TEXT("다시 끊은 연결 수"), UP1MontageSectionLibrary::UnlinkSections(Montage), 0);
    TestEqual(TEXT("빈 몽타주"), UP1MontageSectionLibrary::UnlinkSections(nullptr), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1WarriorComboMontageTest,
    "P1.Editor.MontageSections.WarriorCombo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1WarriorComboMontageTest::RunTest(const FString& Parameters)
{
    using namespace P1MontageSectionLibraryTest;

    const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, COMBO_MONTAGE);
    if (TestNotNull(TEXT("전사 콤보 몽타주"), Montage) == false)
        return false;

    // 섹션 경계는 원래 시퀀스 넷의 길이이고, 섹션끼리 잇지 않는다.
    TestSectionsFollowSequences(*this, *Montage, LoadSequences());
    if (Montage->CompositeSections.Num() != COMBO_STEP_COUNT)
        return false;

    // 섹션마다 타격이 하나이고, 입력 창은 섹션 안에서 끝난다. 다음 단계가 없는 마지막 섹션에는 입력 창이 없다.
    for (int32 Index = 0; Index < COMBO_STEP_COUNT; ++Index)
    {
        const FString What = Montage->GetSectionName(Index).ToString();
        const TArray<const FAnimNotifyEvent*> Hits = NotifiesInSection<UP1AnimNotify_AttackHit>(*Montage, Index);
        const TArray<const FAnimNotifyEvent*> Windows = NotifiesInSection<UP1AnimNotifyState_ComboWindow>(*Montage, Index);

        if (TestEqual(What + TEXT("의 타격 노티파이 수"), Hits.Num(), 1))
            TestEqual(What + TEXT("의 타격 시점"), Hits[0]->GetTriggerTime(), COMBO_HIT_TIMES[Index], TIME_TOLERANCE);

        const bool bHasNextStep = Index < COMBO_STEP_COUNT - 1;
        if (TestEqual(What + TEXT("의 콤보 입력 창 수"), Windows.Num(), bHasNextStep ? 1 : 0) == false || bHasNextStep == false)
            continue;

        float SectionStart = 0.f;
        float SectionEnd = 0.f;
        Montage->GetSectionStartAndEndTime(Index, SectionStart, SectionEnd);
        TestEqual(What + TEXT("의 콤보 입력 창 시작"), Windows[0]->GetTriggerTime(), COMBO_WINDOW_STARTS[Index], TIME_TOLERANCE);
        TestEqual(What + TEXT("의 콤보 입력 창 끝"), Windows[0]->GetEndTriggerTime(), COMBO_WINDOW_ENDS[Index], TIME_TOLERANCE);

        // 입력 창은 그 단계의 입력을 받는 구간이므로 다음 섹션이 시작되기 전에 닫혀야 한다.
        TestTrue(What + TEXT("의 콤보 입력 창이 섹션 안에서 끝남"), Windows[0]->GetEndTriggerTime() < SectionEnd);
    }

    TestEqual(TEXT("재생 속도"), Montage->RateScale, 1.f);
    return true;
}

#endif
