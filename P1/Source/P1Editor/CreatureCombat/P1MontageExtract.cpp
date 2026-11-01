#include "CreatureCombat/P1MontageExtract.h"
#include "Animation/AnimMontage.h"
#include "Game/Combat/P1AnimNotifyState_ComboWindow.h"
#include "Game/Combat/P1AnimNotify_AttackHit.h"

namespace
{
    /** 섹션의 [Start, End) 구간에서 루트 모션을 1/ROOT_MOTION_SAMPLE_RATE초 간격으로 쌓는다. 마지막 표본은 섹션 끝이다. */
    TArray<FVector> SampleRootMotion(const UAnimMontage& Montage, float Start, float End, const FTransform& MeshToActor)
    {
        constexpr float INTERVAL = 1.f / FP1CreatureCombatCheck::ROOT_MOTION_SAMPLE_RATE;
        const float Length = End - Start;
        // 끝이 표본 간격에 딱 맞으면 표본을 하나 더 만들지 않는다. 엔진의 시간은 float라 간격의 배수가 정확히 떨어지지 않는다.
        const int32 Count = FMath::CeilToInt32(Length / INTERVAL - KINDA_SMALL_NUMBER);

        TArray<FVector> Samples;
        Samples.Add(FVector::ZeroVector);

        // 엔진이 틱마다 하듯 구간마다 꺼낸 루트 모션을 이어 붙인다. 회전이 있는 루트 모션도 같은 순서로 쌓여야 런타임 위치와 같다.
        FTransform Accumulated = FTransform::Identity;
        float Previous = Start;
        for (int32 Index = 1; Index <= Count; ++Index)
        {
            const float Current = Index == Count ? End : Start + Index * INTERVAL;
            Accumulated = Montage.ExtractRootMotionFromTrackRange(Previous, Current, FAnimExtractContext()) * Accumulated;
            Previous = Current;

            // 루트 모션은 메시 공간이다. 캐릭터 로컬로 옮기려면 메시의 상대 회전과 크기를 적용한다(위치는 이동량이 아니다).
            Samples.Add(MeshToActor.TransformVector(Accumulated.GetTranslation()));
        }
        return Samples;
    }
}

bool P1MontageExtract::ExtractSection(const UAnimMontage& Montage, FName Section, const FTransform& MeshToActor, FP1CreatureCombatCheck::FAction& OutAction)
{
    const int32 SectionIndex = Montage.GetSectionIndex(Section);
    if (SectionIndex == INDEX_NONE)
        return false;

    float Start = 0.f;
    float End = 0.f;
    Montage.GetSectionStartAndEndTime(SectionIndex, Start, End);
    const bool bLastSection = SectionIndex == Montage.CompositeSections.Num() - 1;
    // 섹션 경계의 노티파이는 뒤 섹션의 시작이다. 마지막 섹션만 끝을 포함한다.
    auto IsInSection = [&](float Time) { return Time >= Start && (Time < End || (bLastSection && Time <= End)); };

    OutAction.Montage = Montage.GetPathName();
    OutAction.Section = Section.ToString();
    OutAction.bHasRootMotion = Montage.HasRootMotion();
    OutAction.RateScale = Montage.RateScale;
    OutAction.Length = End - Start;
    OutAction.HitTimes.Reset();
    OutAction.ComboWindows.Reset();
    OutAction.RootMotion.Reset();

    // 몽타주 트랙의 노티파이만 읽는다. 시퀀스 안에 둔 노티파이는 몽타주가 재생해도 서버가 모른다.
    for (const FAnimNotifyEvent& Notify : Montage.Notifies)
    {
        const float Time = Notify.GetTriggerTime();
        if (IsInSection(Time) == false)
            continue;

        if (Cast<UP1AnimNotify_AttackHit>(Notify.Notify) != nullptr)
            OutAction.HitTimes.Add(Time - Start);
        // 입력 창은 시작한 섹션의 것이다. 섹션 끝을 넘는지는 검사가 본다.
        else if (Cast<UP1AnimNotifyState_ComboWindow>(Notify.NotifyStateClass) != nullptr)
            OutAction.ComboWindows.Add({ Time - Start, Notify.GetEndTriggerTime() - Start });
    }
    OutAction.HitTimes.Sort();
    OutAction.ComboWindows.Sort([](const FP1CreatureCombatCheck::FComboWindow& A, const FP1CreatureCombatCheck::FComboWindow& B) { return A.Start < B.Start; });

    if (OutAction.bHasRootMotion)
        OutAction.RootMotion = SampleRootMotion(Montage, Start, End, MeshToActor);
    return true;
}
