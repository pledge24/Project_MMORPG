// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginWidget.h"
#include "P1.h"
#include "LoginModeBase.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"

// 클래스 열거형 -> 직업 이름으로 바꾸기 위한 맵
TMap<Protocol::CharacterClass, FString> ClassMap = {
    {Protocol::CharacterClass::CLASS_TYPE_KNIGHT, FString(TEXT("전사"))}
    //
};

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (WidgetSwitcher)
    {
        WidgetSwitcher->SetActiveWidgetIndex(0); // 0번 위젯으로 시작
    }
}

void ULoginWidget::SetResultText(bool bSuccess, const FString& Message)
{
    FLinearColor Color = bSuccess ? FLinearColor::White : FLinearColor::Red;

    ResultText->SetColorAndOpacity(FSlateColor(Color));
    ResultText->SetText(FText::FromString(Message));
}

void ULoginWidget::FetchCharacterOverviews(Protocol::S_LOGIN& pkt)
{
    _Characters.Empty();

    // 언리얼 엔진에서 사용할 수 있는 형식으로 변경
    for (auto& Character : pkt.characters())
    {
        FCharacterOverview character;
        character.CharacterId = Character.character_id();
        character.CharacterClass = ClassMap[Character.class_()];
        character.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
        character.CharacterLevel = Character.level();

        _Characters.Add(character);
    }

    OnDisplayCharacterOverviews(_Characters);
}

void ULoginWidget::AddCharacterOverview(Protocol::S_CREATE_CHARACTER& pkt)
{
    if (pkt.success() == false)
    {
        FString Cause = UTF8_TO_TCHAR(pkt.cause().c_str());
        CC_DescriptionText->SetText(FText::FromString(Cause));
        return;
    }

    // Add New CharacterOverview
    {
        FCharacterOverview character;
        character.CharacterId = pkt.characterid();
        character.CharacterClass = ClassMap[Protocol::CharacterClass(CC_CharacterClassId)];
        character.CharacterName = CC_CharacterNameText->GetText().ToString();
        character.CharacterLevel = 1;

        _Characters.Add(character);
    }

    OnDisplayCharacterOverviews(_Characters);
}

void ULoginWidget::RemoveCharacterOverview(Protocol::S_DELETE_CHARACTER& pkt)
{
    bool Success = pkt.success();
    if (Success == false)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("서버 오류: 캐릭터 삭제 실패")));
        return;
    }

    int64 CharacterId = pkt.characterid();
    if (_Characters[LastClickedSlotIdx].CharacterId == CharacterId)
    {
        _Characters.RemoveAt(LastClickedSlotIdx);
    }
    else /* 방어 코드 */
    {
        for (int32 i = 0; i < _Characters.Num(); i++)
        {
            FCharacterOverview& Character = _Characters[i];
            if (Character.CharacterId == CharacterId)
            {
                _Characters.RemoveAt(i);
                break;
            }
        }
    }

    OnDisplayCharacterOverviews(_Characters);
}

void ULoginWidget::SendLoginRequest(FString Username, FString Password)
{
    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestLogin(Username, Password);
        }
    }
}

void ULoginWidget::SendRegisterRequest(FString Username, FString Password)
{
    if (ALoginModeBase* Mode = Cast<ALoginModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        ULoginManager* Manager = Mode->GetLoginManager();
        if (Manager)
        {
            Manager->RequestRegister(Username, Password);
        }
    }
}

void ULoginWidget::SendCreateCharacterPkt(FString CharacterName, int32 CharacterClassId)
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

    Protocol::CharacterOverview* character = new Protocol::CharacterOverview();
    character->set_class_((Protocol::CharacterClass)CharacterClassId);
    character->set_name(TCHAR_TO_UTF8(*CharacterName));

    Protocol::C_CREATE_CHARACTER pkt;
    pkt.set_allocated_character(character);

    SEND_PACKET(pkt);
}

void ULoginWidget::SendDeleteCharacterPkt()
{
    if (LastClickedSlotIdx >= _Characters.Num() || LastClickedSlotIdx < 0)
    {
        CC_DescriptionText->SetText(FText::FromString(TEXT("삭제할 캐릭터가 없습니다.")));
        return;
    }

    FCharacterOverview& character = _Characters[LastClickedSlotIdx];

    Protocol::C_DELETE_CHARACTER pkt;
    pkt.set_characterid(character.CharacterId);

    SEND_PACKET(pkt);
}


