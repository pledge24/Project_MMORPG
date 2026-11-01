#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "P1MontageSectionLibrary.generated.h"

class UAnimMontage;
class UAnimSequenceBase;

/**
 * 몽타주의 섹션을 만드는 에디터 함수 모음.
 *
 * 엔진이 몽타주 섹션을 Python에 노출하지 않으므로, 에디터 Python 스크립트가 몽타주를 만들 때 부른다.
 */
UCLASS()
class UP1MontageSectionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 시퀀스들을 몽타주의 첫 슬롯 트랙에 차례로 이어 붙이고, 시퀀스마다 그 시작에 섹션을 둔다. 섹션끼리는 잇지 않는다.
     * 있던 세그먼트와 섹션은 지운다. 빈 칸, 개수 불일치, 겹친 이름, 다른 스켈레톤이면 몽타주를 바꾸지 않고 false.
     */
    UFUNCTION(BlueprintCallable, Category = "P1|Montage")
    static bool SetSequenceSections(UAnimMontage* Montage, const TArray<UAnimSequenceBase*>& Sequences, const TArray<FName>& SectionNames);
};
