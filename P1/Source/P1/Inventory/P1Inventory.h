#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "Data/P1ItemData.h"
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

    void Rep_SlotChanged(const Protocol::Slot& Slot_, bool OnUse = false);

private:
    /** 카테고리별로 슬롯을 모아 둔 조회용 표다. */
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;

    //~ Debug
private:
    void PrintInventoryData();
};
