#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Shop.generated.h"

class IP1ShopScreen;
class UPrimitiveComponent;

/**
 * 레벨에 놓인 상점이다. 내 플레이어가 상점 범위에 들어오면 상점 창을 열고, 범위에서 나가면 닫는다.
 * 상점 범위는 블루프린트가 소유한 ShopCollision 컴포넌트다.
 */
UCLASS()
class P1_API AP1Shop : public AActor
{
    GENERATED_BODY()

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Shop Range
private:
    UFUNCTION()
    void HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    /** 누가 나가든 상점 창을 닫는다. 블루프린트와 같은 동작이고 결함으로 기록했다. */
    UFUNCTION()
    void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    /** 첫 로컬 플레이어의 컨트롤러가 구현한 창구다. 없으면 nullptr. */
    IP1ShopScreen* FindShopScreen() const;

    /** 블루프린트에서 상점 범위로 쓰는 컴포넌트의 이름이다. */
    static const FName RangeComponentName;
};
