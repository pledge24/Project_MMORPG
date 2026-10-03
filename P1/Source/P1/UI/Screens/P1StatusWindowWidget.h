#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "P1StatusWindowWidget.generated.h"

class AP1MyPlayer;
class UButton;
class UTextBlock;
class UP1MyPlayerData;

UCLASS()
class P1_API UP1StatusWindowWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Equipped Gear
public:
    void UpdateSlotWidget(const Protocol::Slot& Slot_);

protected:
    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Helmet;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Chest;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Arms;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Legs;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Boots;

    UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite, Category = "EquippedGear")
    TObjectPtr<UP1SlotWidget> Equipped_Weapon;

    /** 장착 해제 응답을 기다리는 동안 참이다. 중복 요청을 막는다. */
    UPROPERTY(VisibleAnywhere)
    bool PendingPacket = false;

private:
    /** 더블클릭한 착용 장비를 벗는다. */
    void HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget);

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
    /** 팁 패널이 보이면 숨기고, 숨어 있으면 보인다. */
    UFUNCTION()
    void ToggleTips();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> Button_Details;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CanvasPanel_Tips;
};
