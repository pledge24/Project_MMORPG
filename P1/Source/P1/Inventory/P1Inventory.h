#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "Data/P1ItemData.h"
#include "P1Inventory.generated.h"

/**
 *
 */
UCLASS()
class P1_API UP1Inventory : public UObject
{
    GENERATED_BODY()

public:
    UP1Inventory();

    void Init(Protocol::Inventory* Inventory_);

    void Rep_SlotChanged(const Protocol::Slot& Slot_, bool OnUse = false);

private:
    // DEBUG
    void PrintInventoryData();

private:
    /** 카테고리별 인벤토리 Lookup 매핑 정보*/
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;
};