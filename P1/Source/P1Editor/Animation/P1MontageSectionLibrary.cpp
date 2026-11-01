#include "Animation/P1MontageSectionLibrary.h"
#include "Animation/AnimMontage.h"

namespace
{
    bool IsValidInput(const UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Sequences, const TArray<FName>& SectionNames)
    {
        if (Montage == nullptr || Sequences.Num() == 0 || Sequences.Num() != SectionNames.Num())
            return false;

        TSet<FName> Names;
        for (int32 Index = 0; Index < Sequences.Num(); ++Index)
        {
            const UAnimSequenceBase* Sequence = Sequences[Index];
            if (Sequence == nullptr || Sequence->GetSkeleton() != Montage->GetSkeleton())
                return false;

            bool bAlreadyInSet = false;
            Names.Add(SectionNames[Index], &bAlreadyInSet);
            if (SectionNames[Index] == NAME_None || bAlreadyInSet)
                return false;
        }
        return true;
    }
}

bool UP1MontageSectionLibrary::SetSequenceSections(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Sequences, const TArray<FName>& SectionNames)
{
    if (IsValidInput(Montage, Sequences, SectionNames) == false)
        return false;

    Montage->Modify();
    if (Montage->SlotAnimTracks.Num() == 0)
        Montage->AddSlot(FAnimSlotGroup::DefaultSlotName);

    TArray<FAnimSegment>& Segments = Montage->SlotAnimTracks[0].AnimTrack.AnimSegments;
    Segments.Reset();
    Montage->CompositeSections.Reset();

    TArray<float> SectionStarts;
    float Start = 0.f;
    for (UAnimSequenceBase* Sequence : Sequences)
    {
        FAnimSegment& Segment = Segments.AddDefaulted_GetRef();
        Segment.SetAnimReference(Sequence, true);
        Segment.StartPos = Start;
        SectionStarts.Add(Start);
        Start += Segment.GetLength();
    }

    // 길이를 프레임으로 맞추기 전에 세그먼트의 프레임 레이트를 몽타주에 반영한다. PostEditChange가 그 갱신을 부른다.
    Montage->PostEditChange();
    Montage->SetCompositeLength(Montage->CalculateSequenceLength());

    // 섹션은 세그먼트가 있어야 그 위치의 세그먼트에 묶인다.
    for (int32 Index = 0; Index < SectionNames.Num(); ++Index)
        Montage->AddAnimCompositeSection(SectionNames[Index], SectionStarts[Index]);

    // 엔진은 새 섹션을 앞 섹션의 다음 섹션으로 잇는다. 이어 두면 입력이 없어도 다음 단계가 재생되므로 끊는다.
    UnlinkSections(Montage);

    Montage->PostEditChange();
    Montage->MarkPackageDirty();
    return true;
}

int32 UP1MontageSectionLibrary::UnlinkSections(UAnimMontage* Montage)
{
    if (Montage == nullptr)
        return 0;

    int32 Unlinked = 0;
    for (FCompositeSection& Section : Montage->CompositeSections)
    {
        if (Section.NextSectionName == NAME_None)
            continue;

        if (Unlinked == 0)
            Montage->Modify();
        Section.NextSectionName = NAME_None;
        ++Unlinked;
    }

    if (Unlinked > 0)
        Montage->MarkPackageDirty();
    return Unlinked;
}
