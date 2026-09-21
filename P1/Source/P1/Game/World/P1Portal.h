#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "P1Portal.generated.h"

UCLASS()
class P1_API AP1Portal : public AActor
{
    GENERATED_BODY()

public:
    AP1Portal();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Room Transfer
protected:
    /** PortalId가 가리키는 룸으로 입장을 요청한다. */
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEnterRoomPacket();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
    int32 PortalId = 0;
};
