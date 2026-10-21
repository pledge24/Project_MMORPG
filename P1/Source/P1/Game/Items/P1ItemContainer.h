#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Game/Items/P1ContainerType.h"
#include "Game/Items/P1Slot.h"
#include "P1ItemContainer.generated.h"

class UP1ItemContainer;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnContainerSlotChanged, const FP1Slot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnContainerReset, const UP1ItemContainer&);

/**
 * 내 플레이어의 컨테이너 하나의 칸 사본.
 *
 * 인벤토리의 구역과 착용 장비가 각자 이 클래스의 인스턴스 하나를 두고, 자기 칸이 바뀌면 구독자에게 알린다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1ItemContainer : public UObject
{
    GENERATED_BODY()

public:
    UP1ItemContainer() = default;

    /** 다른 멤버 함수보다 먼저 한 번 부른다. */
    void Init(EP1ContainerType InContainerType);

    EP1ContainerType GetContainerType() const { return ContainerType; }

    /**
     * 칸 전체를 InSlots로 바꾸고 OnReset을 알린다. InSlots의 칸 번호가 이후 유효한 칸 번호가 된다.
     * 주소의 컨테이너는 보지 않고 이 컨테이너로 채운다.
     */
    void ResetSlots(const TArray<FP1Slot>& InSlots);

    /** 칸을 모두 지우고 OnReset을 알린다. */
    void Clear();

    /** 칸 하나를 Change의 내용으로 바꾸고 OnSlotChanged를 알린다. 이 컨테이너의 유효한 칸이 아니면 바꾸지도 알리지도 않고 false. */
    bool ApplySlotChange(const FP1Slot& Change);

    /** 유효한 칸 번호가 아니면 nullptr. */
    const FP1Slot* FindSlot(int32 SlotIndex) const;

    /** 칸 번호 오름차순. */
    const TArray<FP1Slot>& GetSlots() const { return Slots; }

    /** 이 컨테이너의 모든 칸에 든 TemplateId 아이템의 개수 합. 없으면 0. */
    int32 CountItem(int32 TemplateId) const;

    /** 칸을 고친 뒤에 알린다. 구독자는 알림 안에서 어느 칸을 읽어도 새 값을 읽는다. */
    FOnContainerSlotChanged OnSlotChanged;

    /** 칸 전체가 바뀌면 알린다. 구독자는 GetSlots로 다시 그린다. */
    FOnContainerReset OnReset;

private:
    FP1Slot* FindMutableSlot(int32 SlotIndex);

    UPROPERTY()
    EP1ContainerType ContainerType = EP1ContainerType::None;

    UPROPERTY()
    TArray<FP1Slot> Slots;
};
