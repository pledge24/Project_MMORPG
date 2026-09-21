#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Portal.generated.h"

UCLASS()
class P1_API AP1Portal : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AP1Portal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEnterRoomPacket();

protected:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
    int32 PortalId = 0;
};
