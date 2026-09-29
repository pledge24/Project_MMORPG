#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1DeathWidget.generated.h"

UCLASS()
class P1_API UP1DeathWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Death Event
public:
    /** 부활까지 남은 시간을 센다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void StartCountdown();

    UFUNCTION()
    void OnMyPlayerDie(AActor* KilledCreature);

    /** 사망 화면을 숨기고 입력을 게임으로 돌려준다. */
    UFUNCTION()
    void OnMyPlayerRespawn(AActor* RespawnedCreature);

protected:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    //~ Respawn
protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendRespawnInTownPacket();

    void SendRespawnPacket(Protocol::RespawnType respawnType);
};
