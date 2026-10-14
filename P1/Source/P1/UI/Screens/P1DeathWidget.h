#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1DeathWidget.generated.h"

class UTextBlock;

/**
 * 내 플레이어가 죽어 있는 동안 뜨는 사망 화면.
 *
 * 마을 리스폰까지 남은 시간을 보여 주고, 시간이 다 되면 마을 리스폰을 요청한다.
 */
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
    UFUNCTION()
    void OnMyPlayerDie(AActor* KilledCreature);

    UFUNCTION()
    void OnMyPlayerRespawn(AActor* RespawnedCreature);

protected:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    //~ Countdown
private:
    void StartCountdown();

    void TickCountdown();

    void HandleCountdownFinished();

    void ShowRemainingSeconds();

    /** 사망한 뒤 마을 리스폰을 요청하기까지 기다리는 초. 요청이 거절되면 같은 간격으로 다시 요청한다. */
    UPROPERTY(EditDefaultsOnly, Category = "Respawn", meta = (ClampMin = "1"))
    int32 TownRespawnDelaySeconds = 10;

    static constexpr float CountdownIntervalSeconds = 1.f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReturnText;

    int32 RemainingSeconds = 0;

    FTimerHandle CountdownTimerHandle;

    //~ Respawn
private:
    void RequestTownRespawn();

    void HandleTownRespawnRejected();

    FTimerHandle RetryTimerHandle;
};
