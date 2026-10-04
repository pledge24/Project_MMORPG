#include "UI/Frontend/P1LoginMenuWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "TimerManager.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Online/P1LoginManager.h"
#include "UI/Frontend/P1CharacterSlotWidget.h"
#include "Utils/LogCategory.h"

// 클래스 열거형 -> 직업 이름으로 바꾸기 위한 맵
static const TMap<Protocol::CharacterClass, FString> ClassEnumToStringMappings = {
    {Protocol::CharacterClass::CLASS_TYPE_WARRIOR, FString(TEXT("전사"))},
    {Protocol::CharacterClass::CLASS_TYPE_MAGE, FString(TEXT("마법사"))}
};

// 서버 데이터에 직업이 더해져도 화면이 멈추지 않도록, 맵에 없는 값은 대체 이름으로 보여 준다.
static FString GetClassDisplayName(Protocol::CharacterClass CharacterClass)
{
    if (const FString* ClassName = ClassEnumToStringMappings.Find(CharacterClass))
        return *ClassName;

    UE_LOG(LogP1UI, Warning, TEXT("이름이 없는 직업 값: %d"), static_cast<int32>(CharacterClass));
    return TEXT("알 수 없음");
}

void UP1LoginMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    LoginButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnLoginButtonClicked);
    RegisterButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnRegisterButtonClicked);

    CS_CreateBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateButtonClicked);
    CS_DeleteBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteButtonClicked);
    CS_GameStartBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnGameStartButtonClicked);

    CC_WarriorBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnWarriorButtonClicked);
    CC_MagicianBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnMagicianButtonClicked);
    CC_CreateBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateConfirmButtonClicked);
    CC_CancelBtn->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnCreateCancelButtonClicked);

    CD_ConfirmButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteConfirmButtonClicked);
    CD_CancelButton->OnClicked.AddDynamic(this, &UP1LoginMenuWidget::OnDeleteCancelButtonClicked);

    for (UWidget* Child : Slots->GetAllChildren())
    {
        UP1CharacterSlotWidget* CharacterSlot = Cast<UP1CharacterSlotWidget>(Child);
        if (CharacterSlot == nullptr)
            continue;

        CharacterSlot->SetSlotId(CharacterSlots.Num());
        CharacterSlot->OnSlotClicked.BindUObject(this, &UP1LoginMenuWidget::SelectSlot);
        CharacterSlots.Add(CharacterSlot);
    }
}

void UP1LoginMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    WidgetSwitcher->SetActiveWidget(LoginScreen);

    // 게임 서버와 연결이 끊겨 돌아왔으면 그 사유를 보여 준다.
    if (UP1GameInstance* GameInstance = GetGameInstance<UP1GameInstance>())
    {
        const FString Notice = GameInstance->ConsumeLoginNotice();
        if (Notice.IsEmpty() == false)
            SetResultText(false, Notice);
    }

    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->OnAuthResult.AddUObject(this, &UP1LoginMenuWidget::SetResultText);
        LoginManager->OnCharacterListReceived.AddUObject(this, &UP1LoginMenuWidget::FetchCharacterOverviews);
        LoginManager->OnCreateCharacterResult.AddUObject(this, &UP1LoginMenuWidget::AddCharacterOverview);
        LoginManager->OnDeleteCharacterResult.AddUObject(this, &UP1LoginMenuWidget::RemoveCharacterOverview);
        LoginManager->OnEnterGameFailed.AddUObject(this, &UP1LoginMenuWidget::ShowEnterGameFailed);
    }
}

UP1LoginManager* UP1LoginMenuWidget::GetLoginManager() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1LoginManager>() : nullptr;
}

void UP1LoginMenuWidget::SetResultText(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void UP1LoginMenuWidget::LockButtonBriefly(UButton* Button, FTimerHandle& LockTimer)
{
    // 잠긴 동안에는 버튼이 눌리지 않으므로 타이머가 겹쳐 걸리지 않는다.
    Button->SetIsEnabled(false);
    GetWorld()->GetTimerManager().SetTimer(LockTimer, FTimerDelegate::CreateWeakLambda(Button, [Button]()
    {
        Button->SetIsEnabled(true);
    }), BUTTON_LOCK_SECONDS, false);
}

void UP1LoginMenuWidget::OnLoginButtonClicked()
{
    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->RequestLogin(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
    }

    LockButtonBriefly(LoginButton, LoginButtonLockTimer);
}

void UP1LoginMenuWidget::OnRegisterButtonClicked()
{
    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->RequestRegister(UsernameBox->GetText().ToString(), PasswordBox->GetText().ToString());
    }

    LockButtonBriefly(RegisterButton, RegisterButtonLockTimer);
}

void UP1LoginMenuWidget::FetchCharacterOverviews(const Protocol::S_LOGIN& pkt)
{
    // 실패 응답은 서버가 비워서 보내므로 슬롯 수가 0이다. 그대로 적용하면 슬롯이 모두 숨은 선택 화면이 뜬다.
    if (pkt.success() == false)
    {
        UE_LOG(LogP1UI, Warning, TEXT("서버가 캐릭터 목록을 불러오지 못함"));
        SetResultText(false, TEXT("캐릭터 목록을 불러오지 못했습니다."));
        return;
    }

    CharacterOverviews.Empty();

    for (auto& Character : pkt.characters())
    {
        FP1CharacterOverview CharacterOverview;
        CharacterOverview.CharacterId = Character.character_id();
        CharacterOverview.CharacterClass = GetClassDisplayName(Character.class_());
        CharacterOverview.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
        CharacterOverview.CharacterLevel = Character.level();

        CharacterOverviews.Add(CharacterOverview);
    }

    // DisplayCharacterOverviews가 보이는 슬롯 수까지만 채우므로 슬롯 수를 먼저 적용한다.
    ApplyCharacterSlotCount(pkt.character_slot_count());
    DisplayCharacterOverviews();
}

void UP1LoginMenuWidget::ApplyCharacterSlotCount(int32 SlotCount)
{
    CharacterSlotCount = SlotCount;

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

int32 UP1LoginMenuWidget::GetVisibleSlotCount() const
{
    return FMath::Min(CharacterSlotCount, CharacterSlots.Num());
}

void UP1LoginMenuWidget::DisplayCharacterOverviews()
{
    for (UP1CharacterSlotWidget* CharacterSlot : CharacterSlots)
        CharacterSlot->Clear();

    // 보이는 슬롯보다 많은 캐릭터는 보이지 않는다.
    const int32 VisibleCount = FMath::Min(CharacterOverviews.Num(), GetVisibleSlotCount());
    for (int32 i = 0; i < VisibleCount; i++)
        CharacterSlots[i]->ShowCharacter(CharacterOverviews[i]);

    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}

void UP1LoginMenuWidget::SelectSlot(int32 SlotIndex)
{
    SelectedSlotIndex = SlotIndex;

    for (int32 i = 0; i < CharacterSlots.Num(); i++)
        CharacterSlots[i]->SetHighlighted(i == SlotIndex);
}

bool UP1LoginMenuWidget::IsSelectedSlotOccupied() const
{
    return CharacterSlots.IsValidIndex(SelectedSlotIndex) && CharacterSlots[SelectedSlotIndex]->HasCharacter();
}

void UP1LoginMenuWidget::ShowDescription(const FText& Message)
{
    CS_Description->SetText(Message);

    // 같은 핸들로 다시 걸면 이전 타이머가 취소되고 처음부터 센다.
    GetWorld()->GetTimerManager().SetTimer(DescriptionTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        CS_Description->SetText(FText::GetEmpty());
    }), DESCRIPTION_SECONDS, false);
}

void UP1LoginMenuWidget::ShowEnterGameFailed()
{
    UE_LOG(LogP1UI, Warning, TEXT("서버가 게임 입장을 거절함"));

    UTextBlock* TargetText = CS_ResultText ? CS_ResultText.Get() : ResultText.Get();
    TargetText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    TargetText->SetText(FText::FromString(TEXT("게임에 입장하지 못했습니다. 캐릭터 데이터를 확인해 주세요.")));
    TargetText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UP1LoginMenuWidget::SendEnterGamePkt()
{
    if (CharacterOverviews.IsValidIndex(SelectedSlotIndex) == false)
    {
        return;
    }

    FP1CharacterOverview& CharacterOverview = CharacterOverviews[SelectedSlotIndex];

    Protocol::C_ENTER_GAME pkt;
    pkt.set_character_id(CharacterOverview.CharacterId);

    FP1PacketSender::Send(this, pkt);
}

void UP1LoginMenuWidget::OnCreateButtonClicked()
{
    // 서버도 생성 요청을 같은 슬롯 수로 막는다. 여기서 먼저 막는 것은 생성 화면을 열지 않기 위해서다.
    if (CharacterOverviews.Num() >= GetVisibleSlotCount())
    {
        ShowDescription(FText::FromString(TEXT("캐릭터가 꽉 차있습니다!")));
        return;
    }

    ResetCharacterCreateScreen();
    WidgetSwitcher->SetActiveWidget(CharacterCreateScreen);
}

void UP1LoginMenuWidget::OnDeleteButtonClicked()
{
    // 확인 화면에는 문구를 띄울 자리가 없으므로 빈 칸은 여기서 거른다.
    if (IsSelectedSlotOccupied())
    {
        WidgetSwitcher->SetActiveWidget(CharacterDeleteScreen);
        return;
    }

    ShowDescription(FText::FromString(TEXT("삭제할 캐릭터를 선택하지 않았습니다.")));
}

void UP1LoginMenuWidget::OnGameStartButtonClicked()
{
    if (IsSelectedSlotOccupied())
    {
        SendEnterGamePkt();
        return;
    }

    ShowDescription(FText::FromString(TEXT("캐릭터를 선택하지 않았습니다.")));
}

void UP1LoginMenuWidget::ResetCharacterCreateScreen()
{
    SelectedClassId = -1;
    SelectedClassText->SetText(FText::FromString(TEXT("선택 안 함")));
    CC_DescriptionText->SetText(FText::GetEmpty());
    CC_CharacterNameText->SetText(FText::GetEmpty());
}

void UP1LoginMenuWidget::SelectClass(int32 ClassId)
{
    SelectedClassId = ClassId;
    SelectedClassText->SetText(FText::FromString(GetClassDisplayName(Protocol::CharacterClass(ClassId))));
}

void UP1LoginMenuWidget::AddCharacterOverview(const Protocol::S_CREATE_CHARACTER& pkt)
{
    if (pkt.success() == false)
    {
        FString Cause = UTF8_TO_TCHAR(pkt.cause().c_str());
        CC_DescriptionText->SetText(FText::FromString(Cause));
        return;
    }

    {
        FP1CharacterOverview CharacterOverview;
        CharacterOverview.CharacterId = pkt.character_id();
        CharacterOverview.CharacterClass = GetClassDisplayName(Protocol::CharacterClass(RequestedClassId));
        CharacterOverview.CharacterName = RequestedCharacterName;
        CharacterOverview.CharacterLevel = 1;

        UE_LOG(LogP1UI, Log, TEXT("캐릭터 요약 수: %d"), CharacterOverviews.Num());
        CharacterOverviews.Add(CharacterOverview);
        UE_LOG(LogP1UI, Log, TEXT("캐릭터 요약 수: %d"), CharacterOverviews.Num());
    }

    DisplayCharacterOverviews();
}

void UP1LoginMenuWidget::SendCreateCharacterPkt(const FString& CharacterName, int32 CharacterClassId)
{
    if (CharacterName.Len() <= 0)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("캐릭터 이름을 입력해주세요.")));
        return;
    }

    if (CharacterClassId == -1)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("직업을 선택하지 않았습니다.")));
        return;
    }

    Protocol::CharacterOverview* CharacterOverview = new Protocol::CharacterOverview();
    CharacterOverview->set_class_((Protocol::CharacterClass)CharacterClassId);
    CharacterOverview->set_name(TCHAR_TO_UTF8(*CharacterName));

    Protocol::C_CREATE_CHARACTER pkt;
    pkt.set_allocated_character(CharacterOverview);

    // 응답에는 캐릭터 id만 온다. 응답을 기다리는 동안 입력 칸이 바뀌어도 서버에 보낸 값을 목록에 넣는다.
    RequestedCharacterName = CharacterName;
    RequestedClassId = CharacterClassId;

    FP1PacketSender::Send(this, pkt);
}

void UP1LoginMenuWidget::OnWarriorButtonClicked()
{
    SelectClass(Protocol::CharacterClass::CLASS_TYPE_WARRIOR);
}

void UP1LoginMenuWidget::OnMagicianButtonClicked()
{
    SelectClass(Protocol::CharacterClass::CLASS_TYPE_MAGE);
}

void UP1LoginMenuWidget::OnCreateConfirmButtonClicked()
{
    SendCreateCharacterPkt(CC_CharacterNameText->GetText().ToString(), SelectedClassId);
}

void UP1LoginMenuWidget::OnCreateCancelButtonClicked()
{
    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}

void UP1LoginMenuWidget::RemoveCharacterOverview(const Protocol::S_DELETE_CHARACTER& pkt)
{
    bool bSuccess = pkt.success();
    if (bSuccess == false)
    {
        WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
        ShowDescription(FText::FromString(TEXT("서버 오류: 캐릭터 삭제 실패")));
        return;
    }

    int64 CharacterId = pkt.character_id();
    if (CharacterOverviews.IsValidIndex(SelectedSlotIndex) && CharacterOverviews[SelectedSlotIndex].CharacterId == CharacterId)
    {
        CharacterOverviews.RemoveAt(SelectedSlotIndex);
    }
    else
    {
        for (int32 i = 0; i < CharacterOverviews.Num(); i++)
        {
            FP1CharacterOverview& CharacterOverview = CharacterOverviews[i];
            if (CharacterOverview.CharacterId == CharacterId)
            {
                CharacterOverviews.RemoveAt(i);
                break;
            }
        }
    }

    DisplayCharacterOverviews();
}

void UP1LoginMenuWidget::SendDeleteCharacterPkt()
{
    if (CharacterOverviews.IsValidIndex(SelectedSlotIndex) == false)
    {
        WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
        ShowDescription(FText::FromString(TEXT("삭제할 캐릭터가 없습니다.")));
        return;
    }

    FP1CharacterOverview& CharacterOverview = CharacterOverviews[SelectedSlotIndex];

    Protocol::C_DELETE_CHARACTER pkt;
    pkt.set_character_id(CharacterOverview.CharacterId);

    FP1PacketSender::Send(this, pkt);
}

void UP1LoginMenuWidget::OnDeleteConfirmButtonClicked()
{
    SendDeleteCharacterPkt();
}

void UP1LoginMenuWidget::OnDeleteCancelButtonClicked()
{
    WidgetSwitcher->SetActiveWidget(CharacterSelectScreen);
}
