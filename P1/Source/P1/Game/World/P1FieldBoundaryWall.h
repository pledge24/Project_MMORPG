#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1FieldBoundaryWall.generated.h"

class AP1AreaVolume;
class UBoxComponent;

/**
 * 지역의 경계를 막는 보이지 않는 벽.
 *
 * 지역 볼륨의 네 변 바깥에 벽을 세워 플레이어가 지역 밖으로 나가지 못하게 한다. 서버의 룸 경계와 같은 곳을 막는다.
 */
UCLASS()
class P1_API AP1FieldBoundaryWall : public AActor
{
    GENERATED_BODY()

public:
    AP1FieldBoundaryWall();

    //~ Begin AActor Interface
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    //~ Boundary Walls
#if WITH_EDITOR
public:
    const AP1AreaVolume* GetAreaVolume() const { return AreaVolume; }

    /** 벽이 막는 안쪽 영역. 맞춘 지역 볼륨과 같아야 한다. Z는 바닥의 높이다. */
    FVector GetFittedCenter() const { return FittedCenter; }
    FVector2D GetFittedHalfExtent() const { return FittedHalfExtent; }
#endif

private:
    /** 지역 볼륨의 경계에 맞춰 벽 넷의 위치와 크기를 정한다. 벽은 액터의 변환과 무관하게 월드 기준으로 선다. */
    void FitToAreaVolume();

    UPROPERTY()
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> TopWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> BottomWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> LeftWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> RightWall;

    //~ Boundary Size
private:
    /** 벽 두께. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallThickness = 100.f;

    /** 벽 높이. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallHeight = 500.f;

#if WITH_EDITORONLY_DATA
    /** 벽이 둘러쌀 지역 볼륨. 볼륨을 옮기거나 키우면 벽도 다시 맞춰진다. 쿠킹한 빌드에는 맞춘 벽만 남는다. */
    UPROPERTY(EditInstanceOnly, Category = "Boundary")
    TObjectPtr<AP1AreaVolume> AreaVolume;

    /** 마지막으로 맞춘 볼륨의 바닥 중심. 배치 추출이 볼륨과 대조한다. */
    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    FVector FittedCenter = FVector::ZeroVector;

    /** 마지막으로 맞춘 볼륨의 X, Y 방향 반경. */
    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    FVector2D FittedHalfExtent = FVector2D::ZeroVector;
#endif
};
