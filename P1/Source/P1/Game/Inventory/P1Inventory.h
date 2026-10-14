#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Inventory.generated.h"

/**
 * 내 플레이어의 인벤토리 사본.
 *
 * 서버의 InventoryComponent에 대응하며, 인벤토리 슬롯의 상태를 클라이언트에 비춰 두고 슬롯을 찾는 창구가 된다.
 */
UCLASS()
class P1_API UP1Inventory : public UObject
{
    GENERATED_BODY()

public:
    UP1Inventory();

    //~ Slots
public:
    /** 다른 멤버 함수보다 먼저 부른다. Inventory_는 소유하지 않고 가리키기만 한다. */
    void Init(Protocol::Inventory* Inventory_);

    /** 인벤토리 슬롯이 아니면 무시한다. 슬롯 번호는 검증하지 않는다. */
    void Rep_SlotChanged(const Protocol::Slot& Slot_);

    /** 종류와 번호에 맞는 칸이 없으면 nullptr. */
    const Protocol::Slot* FindSlot(Protocol::SlotType Type, int32 SlotId) const;

private:
    /** 슬롯 종류별로, 슬롯 번호를 인덱스로 슬롯을 가리키는 조회용 표. */
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;
};
