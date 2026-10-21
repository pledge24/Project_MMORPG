#pragma once
#include "Game/Data/Templates.h"

/** 기획표 파일 이름. */
namespace GamedataFile
{
    constexpr string_view WARRIOR_LEVELS = "S_Warrior_Level_Data.json";
    constexpr string_view ITEMS = "S_Item.json";
    constexpr string_view MAPS = "S_Map.json";
    constexpr string_view MONSTERS = "S_Monster.json";
    constexpr string_view CURRENCIES = "S_Currency.json";
    constexpr string_view CHARACTER_CREATION = "S_Character_Creation.json";
    constexpr string_view INVENTORY_SECTIONS = "S_Inventory_Section.json";
    constexpr string_view QUICK_SLOTS = "S_Quick_Slot.json";
}

/** 파싱만 마친 기획표 문서. 파일 하나가 행 배열 하나다. */
struct GamedataDocuments
{
    Json warriorLevels;
    Json items;
    Json maps;
    Json monsters;
    Json currencies;
    Json characterCreation;
    Json inventorySections;
    Json quickSlots;
};

/**
 * 기획표 문서를 검증해 템플릿으로 바꾸는 자유 함수.
 * 필드가 없거나 타입이 틀린 행, 표끼리 맞지 않는 값을 부팅 단계에서 걸러 낸다.
 */
namespace GamedataParser
{
    /**
     * 통과하면 nullopt, 실패하면 처음 걸린 오류를 "파일 N번째 행(templateId X): 사유" 형식으로 돌려준다.
     * 실패하면 tables에 일부만 채워졌을 수 있으므로 쓰지 않는다.
     */
    optional<string> Parse(const GamedataDocuments& documents, OUT GamedataTables& tables);
}
