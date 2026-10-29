#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "P1CreatureCombatExtractCommandlet.generated.h"

/**
 * 몬스터와 직업의 에셋에서 크리처 판정 데이터를 읽어 서버가 읽는 파일을 쓰는 커맨드렛.
 *
 * 캡슐 크기와 공격 몽타주의 타이밍, 루트 모션을 서버가 읽는 형식으로 옮기고, 에셋과 기획 데이터가 맞지 않으면 산출물을 바꾸지 않는다.
 */
UCLASS()
class UP1CreatureCombatExtractCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UP1CreatureCombatExtractCommandlet();

    //~ Begin UCommandlet Interface
public:
    virtual int32 Main(const FString& Params) override;
    //~ End UCommandlet Interface
};
