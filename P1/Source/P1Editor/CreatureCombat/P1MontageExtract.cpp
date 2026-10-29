#include "CreatureCombat/P1MontageExtract.h"
#include "Animation/AnimMontage.h"
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

FP1CreatureCombatCheck::FAttack P1MontageExtract::Extract(const UAnimMontage& Montage, const FTransform& MeshToActor)
{
    FP1CreatureCombatCheck::FAttack Attack;
    Attack.Montage = Montage.GetPathName();
    Attack.bHasRootMotion = Montage.HasRootMotion();
    Attack.RateScale = Montage.RateScale;

    for (int32 SectionIndex = 0; SectionIndex < Montage.CompositeSections.Num(); ++SectionIndex)
    {
        float Start = 0.f;
        float End = 0.f;
        Montage.GetSectionStartAndEndTime(SectionIndex, Start, End);
        const bool bLastSection = SectionIndex == Montage.CompositeSections.Num() - 1;

        FP1CreatureCombatCheck::FSection& Section = Attack.Sections.AddDefaulted_GetRef();
        Section.Name = Montage.GetSectionName(SectionIndex).ToString();
        Section.Length = End - Start;

        // 몽타주 트랙의 노티파이만 읽는다. 시퀀스 안에 둔 타격 노티파이는 몽타주가 재생해도 서버가 모른다.
        for (const FAnimNotifyEvent& Notify : Montage.Notifies)
        {
            if (Cast<UP1AnimNotify_AttackHit>(Notify.Notify) == nullptr)
                continue;

            // 섹션 경계의 노티파이는 뒤 섹션의 시작이다. 마지막 섹션만 끝을 포함한다.
            const float Time = Notify.GetTriggerTime();
            if (Time >= Start && (Time < End || (bLastSection && Time <= End)))
                Section.HitTimes.Add(Time - Start);
        }
        Section.HitTimes.Sort();

        if (Attack.bHasRootMotion)
            Section.RootMotion = SampleRootMotion(Montage, Start, End, MeshToActor);
    }

    return Attack;
}
