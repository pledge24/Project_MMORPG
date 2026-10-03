#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1CreatureBoundWidget.h"
#include "P1NameplateWidget.generated.h"

class AP1Player;
class AP1Monster;
class UTextBlock;
class UP1ProgressBarWidget;

UCLASS()
class P1_API UP1NameplateWidget : public UP1UserWidget, public IP1CreatureBoundWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Begin IP1CreatureBoundWidget Interface
public:
    /** 크리처의 종류를 보고 플레이어용과 몬스터용 중 하나로 초기화한다. */
    virtual void BindCreature(AP1Creature* Creature) override;
    //~ End IP1CreatureBoundWidget Interface

    //~ Target Binding
public:
    void InitializePlayerNameplate(AP1Player* TargetPlayer);
    void InitializeMonsterNameplate(AP1Monster* TargetMonster);

    //~ Display
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UTextBlock> NameTextBlock;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UP1ProgressBarWidget> HpBar;

private:
    /** WBP에 지정된 최초 표시 상태다. 되돌릴 때 쓴다. */
    ESlateVisibility NameTextInitVisibility;
    ESlateVisibility HpBarInitVisibility;
};
