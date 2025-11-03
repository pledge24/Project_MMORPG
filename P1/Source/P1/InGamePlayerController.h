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
class UShopWidget;
class UNameplateWidget;
class UNameplateManager;

UENUM(BlueprintType)
enum class WidgetType : uint8
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
class P1_API AInGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AInGamePlayerController() = default;
    ~AInGamePlayerController() = default;

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    /** 위젯 토글 관련*/
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();

public:
    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOnWidget(WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOffWidget(WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    bool IsTurnOnThisWidget(WidgetType Type) const;

    void ToggleWidget(WidgetType Type);

    void AttachNameplate(AActor* Actor);
    void DetachNameplate(AActor* Actor);

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
    TSubclassOf<UShopWidget> ShopWidgetClass;

    UPROPERTY()
    UShopWidget* ShopWidget;

private:
    TMap<WidgetType, UUserWidget*> WidgetMappings;
    int32 WidgetFlag = 0;
    int32 CurrentMaxZOrder = 0;
};
