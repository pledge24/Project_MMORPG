#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Equipment.generated.h"

/**
 * 내 플레이어의 장착 장비 사본. 서버의 EquipmentComponent에 대응한다.
 * UActorComponent가 아니라 UObject이므로 이름에 Component를 붙이지 않는다.
 */
UCLASS()
class P1_API UP1Equipment : public UObject
{
    GENERATED_BODY()

public:
    UP1Equipment() = default;

    //~ Equipped Slots
public:
    void Init(google::protobuf::Map<int32, Protocol::Slot>* EquippedGear_);

    void Rep_SlotChanged(const Protocol::Slot& Slot_);
    const google::protobuf::Map<int32, Protocol::Slot>& GetAllSlot() { return *EquippedGearLookup; }

private:
    /** 장착 슬롯을 가리킨다. 실제 데이터는 MyPlayerData가 갖는다. */
    google::protobuf::Map<int32, Protocol::Slot>* EquippedGearLookup;
};
