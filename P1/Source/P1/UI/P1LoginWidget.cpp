#include "UI/P1LoginWidget.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "P1.h"
#include "Core/P1LoginMenuPlayerController.h"
#include "Online/P1LoginManager.h"
#include "Utils/LogCategory.h"

// 클래스 열거형 -> 직업 이름으로 바꾸기 위한 맵
TMap<Protocol::CharacterClass, FString> ClassEnumToStringMappings = {
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
}

void UP1LoginWidget::SetResultText(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void UP1LoginWidget::FetchCharacterOverviews(Protocol::S_LOGIN& pkt)
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

void UP1LoginWidget::AddCharacterOverview(Protocol::S_CREATE_CHARACTER& pkt)
{
    if (pkt.success() == false)
    {
        FString Cause = UTF8_TO_TCHAR(pkt.cause().c_str());
        CC_DescriptionText->SetText(FText::FromString(Cause));
        return;
    }

    // Add New CharacterOverview
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

void UP1LoginWidget::RemoveCharacterOverview(Protocol::S_DELETE_CHARACTER& pkt)
{
    bool Success = pkt.success();
    if (Success == false)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("서버 오류: 캐릭터 삭제 실패")));
        return;
    }

    int64 CharacterId = pkt.character_id();
    if (CharacterOverviews[LastClickedSlotIdx].CharacterId == CharacterId)
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
    if (AP1LoginMenuPlayerController* Controller = GetP1PlayerController<AP1LoginMenuPlayerController>())
    {
        UP1LoginManager* Manager = Controller->GetLoginManager();
        if (Manager)
        {
            Manager->RequestLogin(Username, Password);
        }
    }
}

void UP1LoginWidget::SendRegisterRequest(FString Username, FString Password)
{
    if (AP1LoginMenuPlayerController* Controller = GetP1PlayerController<AP1LoginMenuPlayerController>())
    {
        UP1LoginManager* Manager = Controller->GetLoginManager();
        if (Manager)
        {
            Manager->RequestRegister(Username, Password);
        }
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

    SEND_PACKET(pkt);
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

    SEND_PACKET(pkt);
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

    SEND_PACKET(pkt);
}


