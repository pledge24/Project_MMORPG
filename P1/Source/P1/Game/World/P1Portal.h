#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Portal.generated.h"

class UPrimitiveComponent;

/**
 * 레벨에 놓인 포털.
 *
 * 내 플레이어가 밟으면 포털이 가리키는 룸으로 옮겨 가게 하고, 행선지를 이름표로 보여 준다.
 */
UCLASS()
class P1_API AP1Portal : public AActor
{
    GENERATED_BODY()

    //~ Begin AActor Interface
public:
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Room Transfer
public:
    /** 포털 표의 Id. 0이면 포털 이동을 요청하지 않는다. */
    int32 GetPortalId() const { return PortalId; }

#if WITH_EDITORONLY_DATA
    /** 서버가 포털 이동을 받아 주는 평면 거리. 배치 추출만 읽는다. */
    double GetUseRadius() const { return UseRadius; }
#endif

private:
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** PortalId 포털의 사용을 요청한다. */
    void RequestRoomTransfer();

    /** 블루프린트에서 밟는 범위로 쓰는 컴포넌트의 이름. */
    static const FName RangeComponentName;

    UPROPERTY(EditAnywhere, Category = "Portal")
    int32 PortalId = 0;

#if WITH_EDITORONLY_DATA
    /**
     * 서버가 포털 이동 요청을 받아 주는, 포털 위치에서의 평면 거리(cm). 밟는 범위보다 넉넉하게 둔다.
     * 도착 지점은 포털 도착 지점 마커로 따로 둔다.
     */
    UPROPERTY(EditAnywhere, Category = "Portal", meta = (ClampMin = "1"))
    float UseRadius = 500.f;
#endif

    //~ Name Tag
private:
    /** 이름표 위젯에 문구와 색을 입힌다. 위젯이 아직 없으면 아무것도 하지 않는다. */
    void ApplyNameTag();

    /** 블루프린트에서 이름표로 쓰는 위젯 컴포넌트의 이름. */
    static const FName NameTagComponentName;

    /** 이름표 문구. 여러 줄이면 줄바꿈을 넣는다. */
    UPROPERTY(EditAnywhere, Category = "Portal|Name Tag")
    FText NameTagText;

    /** 이름표 글자 색. 기본값은 마을로 가는 포털의 색. */
    UPROPERTY(EditAnywhere, Category = "Portal|Name Tag")
    FLinearColor NameTagColor = FLinearColor(0.028834f, 0.027891f, 0.371875f, 1.f);
};
