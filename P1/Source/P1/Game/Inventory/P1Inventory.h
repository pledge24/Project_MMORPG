#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Inventory.generated.h"

UCLASS()
class P1_API UP1Inventory : public UObject
{
    GENERATED_BODY()

public:
    UP1Inventory();

    //~ Slots
public:
    void Init(Protocol::Inventory* Inventory_);

    void Rep_SlotChanged(const Protocol::Slot& Slot_);

    /** 종류와 번호에 맞는 칸이 없으면 nullptr을 돌려준다. */
    const Protocol::Slot* FindSlot(Protocol::SlotType Type, int32 SlotId) const;

private:
    /** 카테고리별로 슬롯을 모아 둔 조회용 표다. */
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;
};
