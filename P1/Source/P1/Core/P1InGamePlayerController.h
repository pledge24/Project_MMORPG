#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/P1WidgetType.h"
#include "P1InGamePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UP1ScreenSubsystem;

/** 인게임 맵의 컨트롤러다. 룸 입장을 요청하고, 화면 서브시스템을 만들고 치우는 시점과 화면 단축키를 넘긴다. */
UCLASS()
class P1_API AP1InGamePlayerController : public APlayerController
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

    //~ Blueprint Widget Control
    // TODO: #135가 블루프린트 호출을 걷어내면 지운다
public:
    /** 화면 서브시스템에 넘긴다. BP_Shop이 부른다. 아래 둘도 같다. */
    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOnWidget(EP1WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void TurnOffWidget(EP1WidgetType Type);

    UFUNCTION(BlueprintCallable, Category = "Widget")
    void DisplayWarningText(const FText& Message);
};
