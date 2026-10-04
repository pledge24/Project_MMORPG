#include "Game/Data/P1ItemData.h"

Protocol::ItemType FP1ItemData::GetItemType() const
{
    // 기획 원본은 열거형 이름에서 접두사를 뺀 값(GEAR)을 적는다. 서버의 Inventory::ToItemType과 같은 규칙이다.
    Protocol::ItemType Result = Protocol::ITEM_TYPE_NONE;
    const FString EnumName = FString(TEXT("ITEM_TYPE_")) + ItemType;
    if (Protocol::ItemType_Parse(TCHAR_TO_UTF8(*EnumName), &Result) == false)
        return Protocol::ITEM_TYPE_NONE;

    return Result;
}
