#include "UI/Frontend/P1LoginWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Online/P1LoginManager.h"
#include "Utils/LogCategory.h"

// 클래스 열거형 -> 직업 이름으로 바꾸기 위한 맵
static const TMap<Protocol::CharacterClass, FString> ClassEnumToStringMappings = {
    {Protocol::CharacterClass::CLASS_TYPE_WARRIOR, FString(TEXT("전사"))},
    {Protocol::CharacterClass::CLASS_TYPE_MAGE, FString(TEXT("마법사"))}
    //
};

void UP1LoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WidgetSwitcher)
    {
        WidgetSwitcher->SetActiveWidgetIndex(0); // 0번 위젯으로 시작
    }

    // 게임 서버와 연결이 끊겨 돌아왔으면 그 사유를 보여 준다.
    if (UP1GameInstance* GameInstance = GetGameInstance<UP1GameInstance>())
    {
        const FString Notice = GameInstance->ConsumeLoginNotice();
        if (Notice.IsEmpty() == false)
            SetResultText(false, Notice);
    }

    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->OnAuthResult.AddUObject(this, &UP1LoginWidget::SetResultText);
        LoginManager->OnCharacterListReceived.AddUObject(this, &UP1LoginWidget::FetchCharacterOverviews);
        LoginManager->OnCreateCharacterResult.AddUObject(this, &UP1LoginWidget::AddCharacterOverview);
        LoginManager->OnDeleteCharacterResult.AddUObject(this, &UP1LoginWidget::RemoveCharacterOverview);
        LoginManager->OnEnterGameFailed.AddUObject(this, &UP1LoginWidget::ShowEnterGameFailed);
    }
}

UP1LoginManager* UP1LoginWidget::GetLoginManager() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1LoginManager>() : nullptr;
}

void UP1LoginWidget::SetResultText(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void UP1LoginWidget::ShowEnterGameFailed()
{
    UE_LOG(LogP1UI, Warning, TEXT("서버가 게임 입장을 거절함"));

    UTextBlock* TargetText = CS_ResultText ? CS_ResultText.Get() : ResultText.Get();
    if (TargetText == nullptr)
        return;

    TargetText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    TargetText->SetText(FText::FromString(TEXT("게임에 입장하지 못했습니다. 캐릭터 데이터를 확인해 주세요.")));
    TargetText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UP1LoginWidget::FetchCharacterOverviews(const Protocol::S_LOGIN& pkt)
{
    CharacterOverviews.Empty();

    // 언리얼 엔진에서 사용할 수 있는 형식으로 변경
    for (auto& Character : pkt.characters())
    {
        FP1CharacterOverview CharacterOverview;
        CharacterOverview.CharacterId = Character.character_id();
        CharacterOverview.CharacterClass = ClassEnumToStringMappings[Character.class_()];
        CharacterOverview.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
        CharacterOverview.CharacterLevel = Character.level();

        CharacterOverviews.Add(CharacterOverview);
    }

    // 위젯에 캐릭터 Overview 진열.
    OnDisplayCharacterOverviews(CharacterOverviews);
}

void UP1LoginWidget::AddCharacterOverview(const Protocol::S_CREATE_CHARACTER& pkt)
{
    if (pkt.success() == false)
    {
        FString Cause = UTF8_TO_TCHAR(pkt.cause().c_str());
        CC_DescriptionText->SetText(FText::FromString(Cause));
        return;
    }

    // 새 캐릭터 요약을 더한다
    {
        FP1CharacterOverview CharacterOverview;
        CharacterOverview.CharacterId = pkt.character_id();
        CharacterOverview.CharacterClass = ClassEnumToStringMappings[Protocol::CharacterClass(CC_CharacterClassId)];
        CharacterOverview.CharacterName = CC_CharacterNameText->GetText().ToString();
        CharacterOverview.CharacterLevel = 1;

        UE_LOG(LogP1UI, Log, TEXT("Character Size :: %d"), CharacterOverviews.Num());
        CharacterOverviews.Add(CharacterOverview);
        UE_LOG(LogP1UI, Log, TEXT("Character Size :: %d"), CharacterOverviews.Num());
    }

    OnDisplayCharacterOverviews(CharacterOverviews);
}

void UP1LoginWidget::RemoveCharacterOverview(const Protocol::S_DELETE_CHARACTER& pkt)
{
    bool Success = pkt.success();
    if (Success == false)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("서버 오류: 캐릭터 삭제 실패")));
        return;
    }

    int64 CharacterId = pkt.character_id();
    if (CharacterOverviews.IsValidIndex(LastClickedSlotIdx) && CharacterOverviews[LastClickedSlotIdx].CharacterId == CharacterId)
    {
        CharacterOverviews.RemoveAt(LastClickedSlotIdx);
    }
    else /* 방어 코드 */
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

    OnDisplayCharacterOverviews(CharacterOverviews);
}

void UP1LoginWidget::SendLoginRequest(FString Username, FString Password)
{
    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->RequestLogin(Username, Password);
    }
}

void UP1LoginWidget::SendRegisterRequest(FString Username, FString Password)
{
    if (UP1LoginManager* LoginManager = GetLoginManager())
    {
        LoginManager->RequestRegister(Username, Password);
    }
}

void UP1LoginWidget::SendEnterGamePkt()
{
    if (LastClickedSlotIdx >= CharacterOverviews.Num() || LastClickedSlotIdx < 0)
    {
        return;
    }

    FP1CharacterOverview& CharacterOverview = CharacterOverviews[LastClickedSlotIdx];

    Protocol::C_ENTER_GAME pkt;
    pkt.set_character_id(CharacterOverview.CharacterId);

    FP1PacketSender::Send(this, pkt);
}

void UP1LoginWidget::SendCreateCharacterPkt(FString CharacterName, int32 CharacterClassId)
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

    FP1PacketSender::Send(this, pkt);
}

void UP1LoginWidget::SendDeleteCharacterPkt()
{
    if (LastClickedSlotIdx >= CharacterOverviews.Num() || LastClickedSlotIdx < 0)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("삭제할 캐릭터가 없습니다.")));
        return;
    }

    FP1CharacterOverview& CharacterOverview = CharacterOverviews[LastClickedSlotIdx];

    Protocol::C_DELETE_CHARACTER pkt;
    pkt.set_character_id(CharacterOverview.CharacterId);

    FP1PacketSender::Send(this, pkt);
}


