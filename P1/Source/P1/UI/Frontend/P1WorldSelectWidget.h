#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1WorldSelectWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UP1AuthClient;
class UP1EntryFlowSubsystem;
class UP1WorldRowWidget;

/**
 * 월드 선택 화면.
 *
 * 인증 클라이언트의 월드 목록을 혼잡도와 함께 보여 주고, 고른 월드로 접속을 요청한다.
 * 화면이 열려 있는 동안 월드 목록을 주기적으로 다시 받는다.
 */
UCLASS()
class P1_API UP1WorldSelectWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    /** 인증 클라이언트의 델리게이트에서 이 위젯을 떼고 목록 갱신을 멈춘다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Screen
public:
    /** 화면을 보일 때 부른다. 남은 문구를 지우고 목록을 그린 뒤 주기적인 목록 갱신을 시작한다. */
    void Open();

    /** 다른 화면으로 넘어갈 때 부른다. 목록 갱신을 멈춘다. */
    void Close();

    /** 월드에 접속하지 못한 사유를 문구 자리에 보여 준다. 다음 접속 요청까지 남는다. */
    void ShowNotice(const FString& Notice);

private:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1AuthClient* GetAuthClient() const;

    /** 게임 인스턴스가 없으면 nullptr. */
    UP1EntryFlowSubsystem* GetEntryFlow() const;

    UFUNCTION()
    void OnConnectButtonClicked();

    UFUNCTION()
    void OnLogoutButtonClicked();

    /** 월드 목록을 다시 받는 주기(초). 게임 서버의 월드 항목 갱신 주기와 같다. */
    static constexpr float REFRESH_SECONDS = 5.0f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConnectButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LogoutButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DescriptionText;

    FTimerHandle RefreshTimer;

    //~ Worlds
private:
    /** 인증 클라이언트의 월드 목록으로 줄을 다시 그린다. 고른 월드가 목록에서 빠졌으면 선택을 푼다. */
    void Refresh();

    /** 접속 버튼이 쓸 월드를 정하고 그 줄을 하이라이트한다. 0이면 선택을 푼다. 접속은 시작하지 않는다. */
    void HighlightWorld(int32 WorldId);

    /** 월드 줄을 담는 패널. 줄은 목록을 받을 때 WorldRowClass로 만들어 채운다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> WorldList;

    /** 접속할 수 있는 월드가 없을 때 보이는 안내. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> EmptyNotice;

    /** 월드 한 줄의 WBP. WBP_WorldSelect의 기본값에서 정한다. */
    UPROPERTY(EditDefaultsOnly, Category = "World Select")
    TSubclassOf<UP1WorldRowWidget> WorldRowClass;

    /** 만들어 둔 줄. 목록이 줄면 남는 줄은 숨겨 두고 다음에 다시 쓴다. */
    UPROPERTY()
    TArray<TObjectPtr<UP1WorldRowWidget>> WorldRows;

    /** 고른 월드가 없으면 0. 목록을 다시 그려도 그 월드가 남아 있으면 바뀌지 않는다. */
    int32 SelectedWorldId = 0;
};
