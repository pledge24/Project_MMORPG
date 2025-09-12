// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Protocol.pb.h"
#include "InGamePlayerController.generated.h"

class UUserWidget;
class UInventoryWidget;
class UStatusWindowWidget;
class UHUDWidget;

/**
 * 
 */
UCLASS()
class P1_API AInGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AInGamePlayerController() = default;
    ~AInGamePlayerController() = default;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

public:
    void OnUpdateInventorySlot(const Protocol::Slot& _Slot);
    void OnUpdateEquippedGearSlot(const Protocol::Slot& _Slot);
    void OnUpdatePlayerUI(const Protocol::PlayerInfo& _PlayerInfo);
    void OnUpdateGold(int32 Gold);

private:
    /** 위젯 토글 관련*/
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();
    void ToggleWidget(UUserWidget* Widget, int32 FlagIdx);

protected:
    /** HUD UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UHUDWidget> HUDWidgetClass;

    UPROPERTY()
    UHUDWidget* HUDWidget;

    /** Control Help UI */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> HelpWidgetClass;

    UPROPERTY()
    UUserWidget* HelpWidget;

    /** 상태창 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UStatusWindowWidget> StatusWindowWidgetClass;

    UPROPERTY()
    UStatusWindowWidget* StatusWindowWidget;

    /** 인벤토리 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY()
    UInventoryWidget* InventoryWidget;

    /** 상점 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> ShopWidgetClass;

    UPROPERTY()
    UUserWidget* ShopWidget;

private:
    int32 ToggleFlag = 0;

};
