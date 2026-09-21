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
	// Sets default values for this actor's properties
	AP1FieldBoundaryWall();

    UFUNCTION(BlueprintCallable, Category = "Boundary")
    void UpdateWalls();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> Root;

    /** 벽 Component */
    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> TopWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> BottomWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> LeftWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    TObjectPtr<UBoxComponent> RightWall;

    // 벽 두께와 높이
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallThickness = 100.f;

    UPROPERTY(EditAnywhere, Category = "Boundary")
    float WallHeight = 500.f;

    // 현재 필드 크기 (한 변 길이)
    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryWidth = 100.f;

    UPROPERTY(EditAnywhere, Category = "Boundary")
    float BoundaryHeight = 100.f;
};
