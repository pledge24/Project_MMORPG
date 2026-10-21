#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Equipment.generated.h"

/**
 * 내 플레이어의 착용 장비 사본.
 *
 * 서버의 EquipmentComponent에 대응하며, 착용 장비의 칸 상태를 클라이언트에 비춰 둔다.
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
    /** 입장 패킷의 소지품에서 착용 장비의 칸을 복사한다. 다른 멤버 함수보다 먼저 부른다. */
    void Init(const Protocol::Possession& Possession);

    /** 착용 장비의 칸이 아니면 무시한다. */
    void Rep_SlotChanged(const Protocol::SlotChange& Change);

    /** 장비 부위(칸 번호)별 칸. */
    const TMap<int32, Protocol::SlotChange>& GetAllSlot() const { return EquippedSlots; }

private:
    TMap<int32, Protocol::SlotChange> EquippedSlots;
};
