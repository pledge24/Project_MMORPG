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
public:
    /** Key가 게임 메뉴를 여닫는 키로 매핑돼 있으면 true. 입력을 혼자 받는 화면이 메뉴 키를 처리할 때 쓴다. */
    bool IsGameMenuKey(const FKey& Key) const;

    /** Key가 채팅 입력칸을 여는 키로 매핑돼 있으면 true. 입력을 혼자 받는 화면이 채팅 키를 처리할 때 쓴다. */
    bool IsOpenChatKey(const FKey& Key) const;

private:
    /** Action이 비었거나 입력 서브시스템이 없으면 false. */
    bool IsKeyMappedToAction(const UInputAction* Action, const FKey& Key) const;

    void OnToggleStatusWindowWidget();
    void OnToggleInventoryWidget();
    void OnToggleGameMenu();
    void OnOpenChat();

    /** 이 로컬 플레이어의 화면 서브시스템. 로컬 플레이어가 없으면 nullptr. */
    UP1ScreenSubsystem* GetScreens() const;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> InGameUIMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleStatusWindowAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleInventoryAction;

    /** 게임 메뉴를 연다. 메뉴나 사망 화면이 떠 있으면 입력을 그 화면이 받으므로, 그때 이 키는 그 화면이 처리한다. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> ToggleGameMenuAction;

    /** 채팅 입력칸을 연다. 입력칸이 열린 동안에는 입력칸이 키를 받으므로, 보내는 Enter는 입력칸이 처리한다. 사망 화면이 떠 있으면 그 화면이 처리한다. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> OpenChatAction;

    //~ Quick Slot Input
private:
    void OnUseQuickSlot(int32 QuickSlotIndex);

    /** 순번이 퀵슬롯 번호다. 0번이 1 키다. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TArray<TObjectPtr<UInputAction>> UseQuickSlotActions;
};
