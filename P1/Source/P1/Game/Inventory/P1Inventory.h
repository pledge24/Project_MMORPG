#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "P1Inventory.generated.h"

/**
 * 내 플레이어의 인벤토리 사본.
 *
 * 서버의 InventoryComponent에 대응하며, 인벤토리 구역의 칸 상태를 클라이언트에 비춰 두고 칸을 찾는 창구가 된다.
 */
UCLASS()
class P1_API UP1Inventory : public UObject
{
    GENERATED_BODY()

public:
    UP1Inventory() = default;

    //~ Slots
public:
    /** 입장 패킷의 소지품에서 인벤토리 구역의 칸을 복사한다. 다른 멤버 함수보다 먼저 부른다. */
    void Init(const Protocol::Possession& Possession);

    /** 인벤토리 구역의 칸이 아니거나 없는 칸 번호면 무시한다. */
    void Rep_SlotChanged(const Protocol::SlotChange& Change);

    /** 컨테이너와 칸 번호에 맞는 칸이 없으면 nullptr. */
    const Protocol::SlotChange* FindSlot(Protocol::ContainerType Container, int32 SlotIndex) const;

    /** 그 구역의 칸을 칸 번호 순서로. 인벤토리 구역이 아니면 nullptr. */
    const TArray<Protocol::SlotChange>* GetSectionSlots(Protocol::ContainerType Container) const;

private:
    /** 구역별로, 칸 번호를 인덱스로 둔 칸 목록. */
    TMap<Protocol::ContainerType, TArray<Protocol::SlotChange>> Sections;
};
