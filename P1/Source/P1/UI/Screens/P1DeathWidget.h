#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1DeathWidget.generated.h"

class UTextBlock;

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

    /** 사망 화면을 숨기고 입력을 게임으로 돌려준다. */
    UFUNCTION()
    void OnMyPlayerRespawn(AActor* RespawnedCreature);

protected:
    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    //~ Countdown
private:
    /** 남은 초를 대기 시간으로 채우고 CountdownIntervalSeconds마다 1씩 줄인다. */
    void StartCountdown();

    void TickCountdown();

    /** 문구를 요청 중으로 바꾸고 마을 리스폰을 요청한다. */
    void HandleCountdownFinished();

    void ShowRemainingSeconds();

    /** 사망한 뒤 마을 리스폰을 요청하기까지 기다리는 초다. 1 이상이다. 1이면 첫 타이머에서 요청한다. */
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
};
