#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1FieldBoundaryWall.generated.h"

class UBoxComponent;

/**
 * 필드 경계를 막는 보이지 않는 벽.
 *
 * 필드의 네 변에 벽을 세워 필드 밖으로 나가지 못하게 한다.
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
public:
    /** 경계 크기 값으로 벽 넷의 위치와 크기를 다시 계산한다. */
    UFUNCTION(BlueprintCallable, Category = "Boundary")
    void UpdateWalls();

private:
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

    /** 필드의 x 방향 길이. 서버 맵 데이터의 depthHalfExtent의 두 배와 같아야 한다. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryDepth = 100.f;

    /** 필드의 y 방향 길이. 서버 맵 데이터의 widthHalfExtent의 두 배와 같아야 한다. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryWidth = 100.f;
};
