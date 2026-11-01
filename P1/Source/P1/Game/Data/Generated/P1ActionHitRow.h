// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Game/Data/Generated/P1AttackAreaShape.h"
#include "P1ActionHitRow.generated.h"

/** ActionHit 표(ActionHit.xlsx)의 한 행. Action 행의 Hits 아래에 키 순서로 실린다. */
USTRUCT(BlueprintType)
struct FP1ActionHitRow
{
    GENERATED_BODY()

    /** 액션 안의 타격 순번. 0부터 빠짐없이 이어지고, 몽타주 섹션의 타격 노티파이 순서다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 HitIndex = 0;

    /** 공격 범위의 평면 형상. 원(CIRCLE), 부채꼴(SECTOR), 직사각형(RECTANGLE) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EP1AttackAreaShape Shape = EP1AttackAreaShape::None;

    /** 원과 부채꼴의 반지름(cm). 0보다 크다. 직사각형은 0 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Radius = 0.f;

    /** 부채꼴의 전체 각도(도). +X를 가운데 둔다. 0 초과 360 이하다. 원과 직사각형은 0 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Angle = 0.f;

    /** 직사각형이 기준점에서 +X로 뻗는 길이(cm). 0보다 크다. 원과 부채꼴은 0 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Length = 0.f;

    /** 직사각형의 폭(cm). 기준점의 좌우로 절반씩이다. 0보다 크다. 원과 부채꼴은 0 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Width = 0.f;

    /** 형상의 기준점. 공격자 기준 앞(+X) 거리(cm) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float OffsetX = 0.f;

    /** 형상의 기준점. 공격자 기준 오른쪽(+Y) 거리(cm) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float OffsetY = 0.f;

    /** 공격자 발밑 기준 높이 범위의 아래(cm) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float ZMin = 0.f;

    /** 공격자 발밑 기준 높이 범위의 위(cm). ZMin보다 크다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float ZMax = 0.f;
};
