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

UCLASS()
class P1_API AP1InGamePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AP1InGamePlayerController() = default;

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Begin APlayerController Interface
protected:
    virtual void SetupInputComponent() override;
    //~ End APlayerController Interface

    //~ Widget Control
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

private:
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();

protected:
    UPROPERTY()
    TMap<EP1WidgetType, TObjectPtr<UUserWidget>> WidgetMappings;

private:
    int32 WidgetFlag = 0;
    int32 CurrentMaxZOrder = 0;
    const int32 DEATH_WIDGET_Z_ORDER = 10000;

    //~ Widget Instances
protected:
    /** HUD UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1HUDWidget> HUDWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1HUDWidget> HUDWidget;

    /** 조작 도움말 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> HelpWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UUserWidget> HelpWidget;

    /** 상태창 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1StatusWindowWidget> StatusWindowWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1StatusWindowWidget> StatusWindowWidget;

    /** 인벤토리 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1InventoryWidget> InventoryWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1InventoryWidget> InventoryWidget;

    /** 상점 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1ShopWidget> ShopWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1ShopWidget> ShopWidget;

    /** 경고 메시지 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1WarningTextWidget> WarningTextWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1WarningTextWidget> WarningTextWidget;

    /** 사망 UI */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UP1DeathWidget> DeathWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UP1DeathWidget> DeathWidget;

    //~ Battle Mode
public:
    void OnToggleBattleMode(bool BattleMode);
};
