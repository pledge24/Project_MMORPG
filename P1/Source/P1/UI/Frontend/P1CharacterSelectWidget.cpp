#include "UI/Frontend/P1CharacterSelectWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "Lobby/P1LobbySubsystem.h"
#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Utils/LogCategory.h"

void UP1CharacterSelectWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    CreateButton->OnClicked.AddDynamic(this, &UP1CharacterSelectWidget::OnCreateButtonClicked);
    DeleteButton->OnClicked.AddDynamic(this, &UP1CharacterSelectWidget::OnDeleteButtonClicked);
    GameStartButton->OnClicked.AddDynamic(this, &UP1CharacterSelectWidget::OnGameStartButtonClicked);
    WorldSelectButton->OnClicked.AddDynamic(this, &UP1CharacterSelectWidget::OnWorldSelectButtonClicked);

    // 패널에 놓인 순서대로 슬롯 번호를 매긴다.
    for (UWidget* Child : Slots->GetAllChildren())
    {
        UP1CharacterSlotWidget* CharacterSlot = Cast<UP1CharacterSlotWidget>(Child);
        if (CharacterSlot == nullptr)
            continue;

        CharacterSlot->SetSlotId(CharacterSlots.Num());
        CharacterSlot->OnSlotClicked.BindUObject(this, &UP1CharacterSelectWidget::SelectSlot);
        CharacterSlots.Add(CharacterSlot);
    }
}

void UP1CharacterSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 응답 구독. 떼는 것은 NativeDestruct가 한다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
    {
        Lobby->OnCharactersChanged.AddUObject(this, &UP1CharacterSelectWidget::Refresh);
        Lobby->OnDeleteCharacterFailed.AddUObject(this, &UP1CharacterSelectWidget::ShowDescription);
        Lobby->OnEnterGameFailed.AddUObject(this, &UP1CharacterSelectWidget::ShowEnterGameFailed);
    }
}

void UP1CharacterSelectWidget::NativeDestruct()
{
    // 서브시스템은 레벨보다 오래 산다. 떼지 않으면 로그인 맵을 다시 열 때마다 죽은 위젯의 항목이 쌓인다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
        Lobby->RemoveListener(this);

    Super::NativeDestruct();
}

void UP1CharacterSelectWidget::Refresh()
{
    // 아래에서 보이는 슬롯 수까지만 채우므로 슬롯 수를 먼저 적용한다.
    ApplyCharacterSlotCount();

    for (UP1CharacterSlotWidget* CharacterSlot : CharacterSlots)
        CharacterSlot->Clear();

    // 보이는 슬롯보다 많은 캐릭터는 보이지 않는다.
    if (UP1LobbySubsystem* Lobby = GetLobby())
    {
        const TArray<FP1LobbyCharacter>& Characters = Lobby->GetCharacters();
        const int32 VisibleCount = FMath::Min(Characters.Num(), GetVisibleSlotCount());
        for (int32 i = 0; i < VisibleCount; i++)
            CharacterSlots[i]->ShowCharacter(Characters[i]);
    }
}

UP1LobbySubsystem* UP1CharacterSelectWidget::GetLobby() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1LobbySubsystem>() : nullptr;
}

void UP1CharacterSelectWidget::ApplyCharacterSlotCount()
{
    const UP1LobbySubsystem* Lobby = GetLobby();
    const int32 CharacterSlotCount = Lobby ? Lobby->GetCharacterSlotCount() : 0;

    // 위젯이 모자라면 넘친 캐릭터를 보여 줄 자리가 없다. 0 이하는 이 필드를 모르는 서버가 보낸 기본값이다.
    if (CharacterSlots.Num() < CharacterSlotCount || CharacterSlotCount <= 0)
    {
        UE_LOG(LogP1UI, Warning, TEXT("슬롯 위젯 수와 서버의 슬롯 수가 맞지 않습니다. 위젯 수: %d, 서버 슬롯 수: %d"), CharacterSlots.Num(), CharacterSlotCount);
    }

    // 보일 슬롯은 숨겼던 것만 되살린다. 디자이너가 정한 표시 상태를 덮어쓰지 않는다.
    const int32 VisibleCount = GetVisibleSlotCount();
    for (int32 i = 0; i < CharacterSlots.Num(); i++)
    {
        if (i >= VisibleCount)
            CharacterSlots[i]->SetVisibility(ESlateVisibility::Collapsed);
        else if (CharacterSlots[i]->GetVisibility() == ESlateVisibility::Collapsed)
            CharacterSlots[i]->SetVisibility(ESlateVisibility::Visible);
    }

    if (SelectedSlotIndex >= VisibleCount)
        SelectSlot(-1);
}

int32 UP1CharacterSelectWidget::GetVisibleSlotCount() const
{
    const UP1LobbySubsystem* Lobby = GetLobby();
    return Lobby ? FMath::Min(Lobby->GetCharacterSlotCount(), CharacterSlots.Num()) : 0;
}

void UP1CharacterSelectWidget::SelectSlot(int32 SlotIndex)
{
    SelectedSlotIndex = SlotIndex;

    for (int32 i = 0; i < CharacterSlots.Num(); i++)
        CharacterSlots[i]->SetHighlighted(i == SlotIndex);
}

const FP1LobbyCharacter* UP1CharacterSelectWidget::GetSelectedCharacter() const
{
    // 슬롯 번호가 곧 목록의 순번이다. Refresh가 목록을 앞 슬롯부터 채운다.
    const UP1LobbySubsystem* Lobby = GetLobby();
    if (Lobby == nullptr || Lobby->GetCharacters().IsValidIndex(SelectedSlotIndex) == false)
        return nullptr;

    return &Lobby->GetCharacters()[SelectedSlotIndex];
}

void UP1CharacterSelectWidget::OnCreateButtonClicked()
{
    // 서버도 생성 요청을 같은 슬롯 수로 막는다. 여기서 먼저 막는 것은 생성 화면을 열지 않기 위해서다.
    UP1LobbySubsystem* Lobby = GetLobby();
    if (Lobby == nullptr || Lobby->GetCharacters().Num() >= GetVisibleSlotCount())
    {
        ShowDescription(FText::FromString(TEXT("빈 캐릭터 슬롯이 없습니다.")));
        return;
    }

    OnCreateRequested.Broadcast();
}

void UP1CharacterSelectWidget::OnDeleteButtonClicked()
{
    // 확인 창에는 이 문구를 띄울 자리가 없으므로 빈 칸은 여기서 거른다.
    if (const FP1LobbyCharacter* Character = GetSelectedCharacter())
    {
        OnDeleteRequested.Broadcast(*Character);
        return;
    }

    ShowDescription(FText::FromString(TEXT("삭제할 캐릭터를 선택하지 않았습니다.")));
}

void UP1CharacterSelectWidget::OnGameStartButtonClicked()
{
    if (const FP1LobbyCharacter* Character = GetSelectedCharacter())
    {
        if (UP1LobbySubsystem* Lobby = GetLobby())
            Lobby->SelectCharacter(Character->CharacterId);
        return;
    }

    ShowDescription(FText::FromString(TEXT("캐릭터를 선택하지 않았습니다.")));
}

void UP1CharacterSelectWidget::OnWorldSelectButtonClicked()
{
    // 로그인 세션이 남아 있으므로 다시 로그인하지 않는다. 화면은 접속 흐름의 단계 알림을 받은 로그인 메뉴가 바꾼다.
    UGameInstance* GameInstance = GetGameInstance();
    if (UP1EntryFlowSubsystem* EntryFlow = GameInstance ? GameInstance->GetSubsystem<UP1EntryFlowSubsystem>() : nullptr)
        EntryFlow->ReturnToWorldSelect();
}

void UP1CharacterSelectWidget::ShowDescription(const FText& Message)
{
    DescriptionText->SetText(Message);

    // 같은 핸들로 다시 걸면 이전 타이머가 취소되고 처음부터 센다.
    GetWorld()->GetTimerManager().SetTimer(DescriptionTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        DescriptionText->SetText(FText::GetEmpty());
    }), DESCRIPTION_SECONDS, false);
}

void UP1CharacterSelectWidget::ShowEnterGameFailed()
{
    ShowDescription(FText::FromString(TEXT("게임에 입장하지 못했습니다. 다시 시도해 주세요.")));
}
