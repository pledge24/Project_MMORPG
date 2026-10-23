#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1GameMenuWidget.generated.h"

class UButton;

/**
 * 게임 중에 Esc로 여는 게임 메뉴.
 *
 * 캐릭터 선택이나 월드 선택으로 돌아가거나 게임을 끄는 창구가 된다.
 * 열려 있는 동안 입력을 이 메뉴만 받고, Esc나 돌아가기 버튼으로 닫는다.
 */
UCLASS()
class P1_API UP1GameMenuWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    //~ End UUserWidget Interface

    //~ Buttons
private:
    UFUNCTION()
    void OnCharacterSelectButtonClicked();

    UFUNCTION()
    void OnWorldSelectButtonClicked();

    UFUNCTION()
    void OnQuitButtonClicked();

    UFUNCTION()
    void OnResumeButtonClicked();

    /** 이 메뉴를 닫는다. 돌아가는 동안에는 로딩 화면이 화면을 덮는다. */
    void CloseMenu();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CharacterSelectButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> WorldSelectButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> QuitButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ResumeButton;
};
