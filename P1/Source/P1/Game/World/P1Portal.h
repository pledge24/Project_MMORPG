#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Portal.generated.h"

class UPrimitiveComponent;

/**
 * 레벨에 놓인 포털이다. 내 플레이어가 밟으면 PortalId가 가리키는 룸으로 룸 이동을 요청한다.
 * 밟는 범위는 블루프린트가 소유한 PortalCollision 컴포넌트다.
 */
UCLASS()
class P1_API AP1Portal : public AActor
{
    GENERATED_BODY()

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Room Transfer
private:
    /** 로컬 플레이어 컨트롤러가 조종하는 폰만 받는다. 원격 플레이어와 몬스터는 무시한다. */
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** PortalId가 가리키는 룸으로 입장을 요청한다. 0이면 요청하지 않는다. */
    void RequestRoomTransfer();

    /** 블루프린트에서 밟는 범위로 쓰는 컴포넌트의 이름이다. */
    static const FName RangeComponentName;

    UPROPERTY(EditAnywhere, Category = "Portal")
    int32 PortalId = 0;
};
