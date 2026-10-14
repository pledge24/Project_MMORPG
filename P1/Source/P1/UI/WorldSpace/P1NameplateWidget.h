#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Entities/P1CreatureBoundWidget.h"
#include "P1NameplateWidget.generated.h"

class AP1Player;
class AP1Monster;
class UTextBlock;
class UP1ProgressBarWidget;

/**
 * 크리처 머리 위에 뜨는 이름판.
 *
 * 월드의 플레이어와 몬스터 위에 이름을, 몬스터에게는 HP도 함께 보여 준다.
 */
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
    /** 플레이어와 몬스터만 받는다. 다른 크리처면 아무것도 하지 않는다. */
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
    /** 대상 크리처의 종류에 따라 정한 최초 표시 상태. */
    ESlateVisibility NameTextInitVisibility;
    ESlateVisibility HpBarInitVisibility;
};
