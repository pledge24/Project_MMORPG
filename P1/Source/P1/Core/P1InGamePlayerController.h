// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Protocol.pb.h"
#include "P1InGamePlayerController.generated.h"

class UUserWidget;
class UP1InventoryWidget;
class UP1StatusWindowWidget;
class UP1HUDWidget;
class UP1ShopWidget;
class UP1NameplateWidget;
class UP1WarningTextWidget;
class UP1DeathWidget;

UENUM(BlueprintType)
enum class EP1WidgetType : uint8
{
    WIDGET_NONE = 0 UMETA(Hidden),
    WIDGET_STATUS_WINDOW = 1 UMETA(DisplayName="StatusWindow"),
    WIDGET_INVENTORY = 2 UMETA(DisplayName = "Inventory"),
    WIDGET_SHOP = 3 UMETA(DisplayName = "Shop"),
};

/**
 * 
 */
UCLASS()
class P1_API AP1InGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AP1InGamePlayerController() = default;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    /** 위젯 토글 관련*/
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();

public:
    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOnWidget(EP1WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOffWidget(EP1WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    bool IsTurnOnThisWidget(EP1WidgetType Type) const;

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void DisplayWarningText(const FText& Message);

    void ToggleWidget(EP1WidgetType Type);

    void OnToggleBattleMode(bool BattleMode);

protected:
    /** HUD UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1HUDWidget> HUDWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1HUDWidget* HUDWidget;

    /** Control Help UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> HelpWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UUserWidget* HelpWidget;

    /** 상태창 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1StatusWindowWidget> StatusWindowWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1StatusWindowWidget* StatusWindowWidget;

    /** 인벤토리 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1InventoryWidget> InventoryWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1InventoryWidget* InventoryWidget;

    /** 상점 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1ShopWidget> ShopWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1ShopWidget* ShopWidget;

    /** 경고 메세지 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1WarningTextWidget> WarningTextWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1WarningTextWidget* WarningTextWidget;

    /** 사망 UI*/
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1DeathWidget> DeathWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UP1DeathWidget* DeathWidget;

protected:
    UPROPERTY()
    TMap<EP1WidgetType, UUserWidget*> WidgetMappings;

private:
    int32 WidgetFlag = 0;
    int32 CurrentMaxZOrder = 0;
    const int32 DEATH_WIDGET_Z_ORDER = 10000;
};
