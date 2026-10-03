#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1EquippedGear.generated.h"

UCLASS()
class P1_API UP1EquippedGear : public UObject
{
    GENERATED_BODY()

public:
    UP1EquippedGear() = default;

    //~ Equipped Slots
public:
    void Init(google::protobuf::Map<int32, Protocol::Slot>* EquippedGear_);

    void Rep_SlotChanged(const Protocol::Slot& Slot_);
    const google::protobuf::Map<int32, Protocol::Slot>& GetAllSlot() { return *EquippedGearLookup; }

private:
    /** 장착 슬롯을 가리킨다. 실제 데이터는 MyPlayerData가 갖는다. */
    google::protobuf::Map<int32, Protocol::Slot>* EquippedGearLookup;
};
