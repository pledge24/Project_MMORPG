#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1StatusWindowWidget.generated.h"

class UButton;
class UTextBlock;
class UP1MyPlayerData;

/**
 * 내 플레이어의 상태 창.
 *
 * 착용한 장비와 세부 능력치를 보여 준다. 착용 장비는 품은 컨테이너 위젯이 그린다.
 */
UCLASS()
class P1_API UP1StatusWindowWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Stat Details
public:
    void UpdateAllStat(UP1MyPlayerData* MyPlayerData);

    void UpdateMaxHp(int64 Value);
    void UpdateMaxMp(int64 Value);
    void UpdatePhysicalAttack(int64 Value);
    void UpdateMagicalAttack(int64 Value);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Details_MaxHp;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Details_MaxMp;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Details_Physical_Attack;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Details_Magical_Attack;

    //~ Tips
private:
    UFUNCTION()
    void ToggleTips();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Button_Details;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CanvasPanel_Tips;
};
