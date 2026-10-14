#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Online/P1CharacterOverview.h"
#include "P1LoginMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UPanelWidget;
class UTextBlock;
class UWidgetSwitcher;
class UP1CharacterSlotWidget;
class UP1LoginManager;

namespace Protocol
{
    class S_LOGIN;
    class S_CREATE_CHARACTER;
    class S_DELETE_CHARACTER;
}

/**
 * 로그인 맵의 메뉴 위젯.
 *
 * 로그인부터 캐릭터 선택, 생성, 삭제를 거쳐 게임 입장을 요청하기까지의 화면을 맡는다.
 */
UCLASS()
class P1_API UP1LoginMenuWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Screen
private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> LoginScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CharacterSelectScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CharacterCreateScreen;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CharacterDeleteScreen;

    //~ Login
private:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1LoginManager* GetLoginManager() const;

    void SetResultText(bool bSuccess, const FString& Message);

    /** 요청이 겹치지 않게 버튼을 BUTTON_LOCK_SECONDS 동안 잠근다. */
    void LockButtonBriefly(UButton* Button, FTimerHandle& LockTimer);

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnRegisterButtonClicked();

    static constexpr float BUTTON_LOCK_SECONDS = 2.0f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> UsernameBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> PasswordBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LoginButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RegisterButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ResultText;

    FTimerHandle LoginButtonLockTimer;
    FTimerHandle RegisterButtonLockTimer;

    //~ Character Select
private:
    /** 로그인 응답의 캐릭터 목록으로 캐릭터 선택 화면을 채우고 연다. */
    void FetchCharacterOverviews(const Protocol::S_LOGIN& pkt);

    /**
     * 서버가 보낸 슬롯 수만큼 슬롯 위젯을 보이고 나머지는 숨긴다.
     * 숨긴 슬롯을 고른 상태였으면 선택을 푼다.
     */
    void ApplyCharacterSlotCount(int32 SlotCount);

    /** 서버가 보낸 슬롯 수와 슬롯 위젯 수 중 작은 값. 캐릭터 생성 가능 여부도 이 값으로 판정한다. */
    int32 GetVisibleSlotCount() const;

    /** 캐릭터 요약을 슬롯에 다시 그리고 캐릭터 선택 화면으로 넘어간다. */
    void DisplayCharacterOverviews();

    /** 슬롯을 고르고 하이라이트한다. -1이면 선택을 푼다. */
    void SelectSlot(int32 SlotIndex);

    bool IsSelectedSlotOccupied() const;

    /** 안내 문구를 띄우고 DESCRIPTION_SECONDS 뒤에 지운다. 새 문구가 뜨면 시간을 다시 센다. */
    void ShowDescription(const FText& Message);

    /** 서버가 게임 입장을 거절하면 캐릭터 선택 화면에 알린다. */
    void ShowEnterGameFailed();

    void SendEnterGamePkt();

    UFUNCTION()
    void OnCreateButtonClicked();

    UFUNCTION()
    void OnDeleteButtonClicked();

    UFUNCTION()
    void OnGameStartButtonClicked();

    static constexpr float DESCRIPTION_SECONDS = 3.0f;

    /** 캐릭터 슬롯 위젯을 담은 패널. 자식의 순서가 슬롯 번호다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> Slots;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CS_Description;

    /** 게임 입장 실패 문구. 없으면 ResultText에 대신 적는다. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CS_ResultText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CS_CreateBtn;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CS_DeleteBtn;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CS_GameStartBtn;

    UPROPERTY()
    TArray<TObjectPtr<UP1CharacterSlotWidget>> CharacterSlots;

    TArray<FP1CharacterOverview> CharacterOverviews;

    /** 계정이 가질 수 있는 캐릭터 수. 캐릭터 목록을 받기 전에는 0. */
    int32 CharacterSlotCount = 0;

    /** 고른 슬롯이 없으면 -1. 캐릭터 목록을 다시 그려도 바뀌지 않는다. */
    int32 SelectedSlotIndex = -1;

    FTimerHandle DescriptionTimer;

    //~ Character Create
private:
    /** 생성 화면으로 넘어가기 전에 불러 직업 선택과 이름 입력을 비운다. */
    void ResetCharacterCreateScreen();

    /** ClassId는 Protocol::CharacterClass 값이다. */
    void SelectClass(int32 ClassId);

    void AddCharacterOverview(const Protocol::S_CREATE_CHARACTER& pkt);
    void SendCreateCharacterPkt(const FString& CharacterName, int32 CharacterClassId);

    UFUNCTION()
    void OnWarriorButtonClicked();

    UFUNCTION()
    void OnMagicianButtonClicked();

    UFUNCTION()
    void OnCreateConfirmButtonClicked();

    UFUNCTION()
    void OnCreateCancelButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CC_DescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> CC_CharacterNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SelectedClassText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CC_WarriorBtn;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CC_MagicianBtn;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CC_CreateBtn;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CC_CancelBtn;

    /** Protocol::CharacterClass 값. 고르지 않았으면 -1. */
    int32 SelectedClassId = -1;

    /** 마지막으로 보낸 생성 요청의 이름. 응답에는 이름이 없으므로 이 값을 목록에 넣는다. */
    FString RequestedCharacterName;

    /** 마지막으로 보낸 생성 요청의 직업(Protocol::CharacterClass 값). */
    int32 RequestedClassId = -1;

    //~ Character Delete
private:
    void RemoveCharacterOverview(const Protocol::S_DELETE_CHARACTER& pkt);
    void SendDeleteCharacterPkt();

    UFUNCTION()
    void OnDeleteConfirmButtonClicked();

    UFUNCTION()
    void OnDeleteCancelButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CD_ConfirmButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CD_CancelButton;
};
