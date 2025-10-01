// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FieldBoundaryWall.generated.h"

class UBoxComponent;

UCLASS()
class P1_API AFieldBoundaryWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFieldBoundaryWall();

    UFUNCTION(BlueprintCallable, Category = "Boundary")
    void UpdateWalls();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY()
    USceneComponent* Root;

    /** 벽 Component */
    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    UBoxComponent* TopWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    UBoxComponent* BottomWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    UBoxComponent* LeftWall;

    UPROPERTY(VisibleAnywhere, Category = "Boundary")
    UBoxComponent* RightWall;

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
