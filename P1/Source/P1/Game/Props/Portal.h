// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

UCLASS()
class P1_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEnterRoomPacket();

protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
    int32 PortalId = 0;
};
