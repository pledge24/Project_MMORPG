#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Equipment.generated.h"

/**
 * 내 플레이어의 장착 장비 사본.
 *
 * 서버의 EquipmentComponent에 대응하며, 장착 슬롯의 상태를 클라이언트에 비춰 둔다.
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
    /** 다른 멤버 함수보다 먼저 부른다. EquippedGear_는 소유하지 않고 가리키기만 한다. */
    void Init(google::protobuf::Map<int32, Protocol::Slot>* EquippedGear_);

    /** 장착 슬롯이 아닌 슬롯은 무시한다. */
    void Rep_SlotChanged(const Protocol::Slot& Slot_);
    const google::protobuf::Map<int32, Protocol::Slot>& GetAllSlot() { return *EquippedGearLookup; }

private:
    /** 장착 슬롯 맵. 실제 데이터는 MyPlayerData가 갖는다. */
    google::protobuf::Map<int32, Protocol::Slot>* EquippedGearLookup;
};
