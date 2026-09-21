#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1FieldBoundaryWall.generated.h"

class UBoxComponent;

UCLASS()
class P1_API AP1FieldBoundaryWall : public AActor
{
    GENERATED_BODY()

public:
    AP1FieldBoundaryWall();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    //~ Boundary Walls
public:
    /** 아래 크기 값으로 벽 넷의 위치와 크기를 다시 계산한다. */
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
    /** 벽 두께다. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallThickness = 100.f;

    /** 벽 높이다. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallHeight = 500.f;

    /** 필드 한 변의 길이다. */
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryWidth = 100.f;

    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryHeight = 100.f;
};
