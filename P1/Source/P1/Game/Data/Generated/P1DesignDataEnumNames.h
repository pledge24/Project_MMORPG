// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Network/Common.pb.h"
#include "Network/Item.pb.h"

/**
 * 셀에 적는 값 이름을 프로토콜 열거형으로 읽는 함수.
 *
 * 데이터 테이블의 행 구조체가 값 이름으로 담은 .proto 열거형을 게임 코드가 쓰는 값으로 바꾼다.
 */
namespace P1DesignData
{
    /** 값 이름(WARRIOR, MAGE)을 Protocol::CharacterClass로 읽는다. 모르는 이름이면 OutValue를 두고 false. */
    inline bool ParseEnumName(const FString& Name, Protocol::CharacterClass& OutValue)
    {
        if (Name.Equals(TEXT("WARRIOR"), ESearchCase::CaseSensitive)) { OutValue = Protocol::CHARACTER_CLASS_WARRIOR; return true; }
        if (Name.Equals(TEXT("MAGE"), ESearchCase::CaseSensitive)) { OutValue = Protocol::CHARACTER_CLASS_MAGE; return true; }
        return false;
    }

    /** 값 이름(HELMET, CHEST, LEGS, ARMS, BOOTS, WEAPON)을 Protocol::GearPart로 읽는다. 모르는 이름이면 OutValue를 두고 false. */
    inline bool ParseEnumName(const FString& Name, Protocol::GearPart& OutValue)
    {
        if (Name.Equals(TEXT("HELMET"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_HELMET; return true; }
        if (Name.Equals(TEXT("CHEST"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_CHEST; return true; }
        if (Name.Equals(TEXT("LEGS"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_LEGS; return true; }
        if (Name.Equals(TEXT("ARMS"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_ARMS; return true; }
        if (Name.Equals(TEXT("BOOTS"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_BOOTS; return true; }
        if (Name.Equals(TEXT("WEAPON"), ESearchCase::CaseSensitive)) { OutValue = Protocol::GEAR_PART_WEAPON; return true; }
        return false;
    }

    /** 값 이름(GEAR, CONSUMABLE, MISCELLANEOUS)을 Protocol::ItemType로 읽는다. 모르는 이름이면 OutValue를 두고 false. */
    inline bool ParseEnumName(const FString& Name, Protocol::ItemType& OutValue)
    {
        if (Name.Equals(TEXT("GEAR"), ESearchCase::CaseSensitive)) { OutValue = Protocol::ITEM_TYPE_GEAR; return true; }
        if (Name.Equals(TEXT("CONSUMABLE"), ESearchCase::CaseSensitive)) { OutValue = Protocol::ITEM_TYPE_CONSUMABLE; return true; }
        if (Name.Equals(TEXT("MISCELLANEOUS"), ESearchCase::CaseSensitive)) { OutValue = Protocol::ITEM_TYPE_MISCELLANEOUS; return true; }
        return false;
    }
}
