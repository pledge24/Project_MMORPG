#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Game/Interaction/P1ShopScreen.h"
#include "P1InGamePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UP1ScreenSubsystem;

/**
 * 인게임 맵의 플레이어 컨트롤러.
 *
 * 인게임 맵에서 플레이어의 화면 입력을 받고, 게임 객체가 요청하는 화면 조작을 화면 쪽에 넘기는 창구가 된다.
 */
UCLASS()
class P1_API AP1InGamePlayerController : public APlayerController, public IP1ShopScreen
{
    GENERATED_BODY()

public:
    AP1InGamePlayerController() = default;

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End AActor Interface

    //~ Begin APlayerController Interface
protected:
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
    //~ End APlayerController Interface

    //~ Begin IP1ShopScreen Interface
public:
    virtual void OpenShopWindow() override;
    virtual void CloseShopWindow() override;
    virtual void ShowShopWarning(const FText& Message) override;
    //~ End IP1ShopScreen Interface

    //~ Screen Input
private:
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();

    /** 이 로컬 플레이어의 화면 서브시스템. 로컬 플레이어가 없으면 nullptr. */
    UP1ScreenSubsystem* GetScreens() const;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> InGameUIMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleStatusWindowAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleInventoryAction;
};
