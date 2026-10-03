#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Game/Interaction/P1ShopScreen.h"
#include "P1InGamePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UP1ScreenSubsystem;

/**
 * 인게임 맵의 컨트롤러다. 룸 입장을 요청하고, 화면 서브시스템을 만들고 치우는 시점과 화면 단축키를 넘긴다.
 * 상점이 부르는 상점 창 여닫기와 경고도 화면 서브시스템에 넘긴다.
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

    //~ Screen Input
private:
    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();

    /** 이 로컬 플레이어의 화면 서브시스템이다. 로컬 플레이어가 없으면 nullptr. */
    UP1ScreenSubsystem* GetScreens() const;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> InGameUIMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleStatusWindowAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleInventoryAction;

    //~ Begin IP1ShopScreen Interface
public:
    virtual void OpenShopWindow() override;
    virtual void CloseShopWindow() override;
    virtual void ShowShopWarning(const FText& Message) override;
    //~ End IP1ShopScreen Interface
};
