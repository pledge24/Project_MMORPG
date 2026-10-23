#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Lobby/P1LobbyCharacter.h"
#include "P1CharacterSelectWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UP1CharacterSlotWidget;
class UP1LobbySubsystem;

DECLARE_MULTICAST_DELEGATE(FOnCharacterCreateRequested);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterDeleteRequested, const FP1LobbyCharacter& /*Character*/);

/**
 * 캐릭터 선택 화면.
 *
 * 로비의 캐릭터 목록을 슬롯에 보여 주고, 고른 캐릭터로 게임 입장을 요청한다.
 * 생성 화면과 삭제 확인 창은 직접 열지 않고 요청 알림으로 로그인 메뉴에 맡긴다.
 */
UCLASS()
class P1_API UP1CharacterSelectWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    /** 로비의 델리게이트에서 이 위젯을 뗀다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Characters
public:
    /** 로비의 캐릭터 목록 사본으로 슬롯을 다시 그린다. */
    void Refresh();

private:
    /** 게임 인스턴스가 없으면 nullptr. */
    UP1LobbySubsystem* GetLobby() const;

    /**
     * 로비가 받은 슬롯 수만큼 슬롯 위젯을 보이고 나머지는 숨긴다.
     * 숨긴 슬롯을 고른 상태였으면 선택을 푼다.
     */
    void ApplyCharacterSlotCount();

    /** 로비가 받은 슬롯 수와 슬롯 위젯 수 중 작은 값. 캐릭터 생성 가능 여부도 이 값으로 판정한다. */
    int32 GetVisibleSlotCount() const;

    /** 슬롯을 고르고 하이라이트한다. -1이면 선택을 푼다. */
    void SelectSlot(int32 SlotIndex);

    /** 고른 슬롯의 캐릭터. 고른 슬롯에 캐릭터가 없으면 nullptr. */
    const FP1LobbyCharacter* GetSelectedCharacter() const;

    /** 캐릭터 슬롯 위젯을 담은 패널. 자식의 순서가 슬롯 번호다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> Slots;

    UPROPERTY()
    TArray<TObjectPtr<UP1CharacterSlotWidget>> CharacterSlots;

    /** 고른 슬롯이 없으면 -1. 캐릭터 목록을 다시 그려도 바뀌지 않는다. */
    int32 SelectedSlotIndex = -1;

    //~ Requests
public:
    /** 빈 슬롯이 있을 때 생성 버튼을 누르면 알린다. */
    FOnCharacterCreateRequested OnCreateRequested;

    /** 캐릭터를 고른 채 삭제 버튼을 누르면 그 캐릭터를 알린다. */
    FOnCharacterDeleteRequested OnDeleteRequested;

private:
    UFUNCTION()
    void OnCreateButtonClicked();

    UFUNCTION()
    void OnDeleteButtonClicked();

    UFUNCTION()
    void OnGameStartButtonClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CreateButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DeleteButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> GameStartButton;

    /** 월드 선택 화면으로 돌아가는 버튼. 월드 선택 화면이 생기기 전까지는 꺼 둔다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> WorldSelectButton;

    //~ Messages
public:
    /** 안내 문구를 띄우고 DESCRIPTION_SECONDS 뒤에 지운다. 새 문구가 뜨면 시간을 다시 센다. */
    void ShowDescription(const FText& Message);

private:
    /** 서버가 게임 입장을 거절하면 알린다. */
    void ShowEnterGameFailed();

    static constexpr float DESCRIPTION_SECONDS = 3.0f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DescriptionText;

    FTimerHandle DescriptionTimer;
};
