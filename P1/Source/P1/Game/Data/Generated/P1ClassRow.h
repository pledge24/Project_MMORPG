// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1DesignDataEnumNames.h"
#include "Network/Common.pb.h"
#include "P1ClassRow.generated.h"

/** Class 표(Class.xlsx)의 한 행. 행 이름은 키(Class)다. */
USTRUCT(BlueprintType)
struct FP1ClassRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Class 값을 프로토콜 열거형으로 읽는다. 값 이름이 아니면 0번 값이다. */
    Protocol::CharacterClass GetClass() const
    {
        Protocol::CharacterClass Parsed{};
        P1DesignData::ParseEnumName(Class, Parsed);
        return Parsed;
    }

    /** 직업 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Class;

    /** 일반 공격 콤보의 첫 단계 액션. 다음 단계는 액션 표의 NextAction이 정한다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 NormalAttackAction = 0;
};
