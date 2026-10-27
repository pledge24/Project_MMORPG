// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/Generated/P1DesignDataEnumNames.h"
#include "Network/Common.pb.h"
#include "Network/Item.pb.h"
#include "P1ItemRow.generated.h"

/** Item 표(Item.xlsx)의 한 행. 행 이름은 키(Id)다. */
USTRUCT(BlueprintType)
struct FP1ItemRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Type 값을 프로토콜 열거형으로 읽는다. 값 이름이 아니면 0번 값이다. */
    Protocol::ItemType GetType() const
    {
        Protocol::ItemType Parsed{};
        P1DesignData::ParseEnumName(Type, Parsed);
        return Parsed;
    }

    /** AllowedClasses 값을 프로토콜 열거형으로 읽는다. 값 이름이 아닌 원소는 0번 값이다. */
    TArray<Protocol::CharacterClass> GetAllowedClasses() const
    {
        TArray<Protocol::CharacterClass> ParsedValues;
        for (const FString& ElementName : AllowedClasses)
        {
            Protocol::CharacterClass Parsed{};
            P1DesignData::ParseEnumName(ElementName, Parsed);
            ParsedValues.Add(Parsed);
        }
        return ParsedValues;
    }

    /** 아이템 번호. 장비 1000번대, 소모품 2000번대, 기타 3000번대다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Id = 0;

    /** 아이템 이름 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Name;

    /** 아이템 종류. 장비는 ItemEquipStat에, 소모품은 ItemConsumeEffect에 행이 있다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Type;

    /** 착용할 수 있는 최소 레벨 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 LevelRequirement = 0;

    /** 착용할 수 있는 직업. 비어 있으면 모든 직업이다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> AllowedClasses;

    /** 상점 구매 가격(골드) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int64 BuyPrice = 0;

    /** 상점 판매 가격(골드) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int64 SellPrice = 0;

    /** 상점에 팔 수 있는지 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool Sellable = false;

    /** 한 칸에 쌓을 수 있는 최대 개수. 1 이상이다 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MaxStack = 0;

    /** 아이템 설명 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FString Description;
};
