#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Game/QuickSlots/P1QuickSlot.h"
#include "P1QuickSlots.generated.h"

class UP1QuickSlots;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuickSlotChanged, const FP1QuickSlot&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuickSlotsReset, const UP1QuickSlots&);

/**
 * 내 플레이어의 퀵슬롯 사본.
 *
 * 서버가 알려 준 퀵슬롯의 대상을 들고, 퀵슬롯을 고친 뒤에 구독자에게 알린다. 아이템을 담지 않으므로 컨테이너가 아니다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1QuickSlots : public UObject
{
    GENERATED_BODY()

public:
    UP1QuickSlots() = default;

    /** 퀵슬롯 전체를 InQuickSlots로 바꾸고 OnReset을 알린다. InQuickSlots의 번호가 이후 유효한 퀵슬롯 번호가 된다. */
    void ResetQuickSlots(const TArray<FP1QuickSlot>& InQuickSlots);

    /** 퀵슬롯을 모두 지우고 OnReset을 알린다. */
    void Clear();

    /** 퀵슬롯 하나를 Change의 대상으로 바꾸고 OnChanged를 알린다. 유효한 퀵슬롯 번호가 아니면 바꾸지도 알리지도 않고 false. */
    bool ApplyChange(const FP1QuickSlot& Change);

    /** 유효한 퀵슬롯 번호가 아니면 nullptr. */
    const FP1QuickSlot* Find(int32 SlotIndex) const;

    /** 번호 오름차순. */
    const TArray<FP1QuickSlot>& GetQuickSlots() const { return QuickSlots; }

    /** 퀵슬롯을 고친 뒤에 알린다. */
    FOnQuickSlotChanged OnChanged;

    /** 퀵슬롯 전체가 바뀌면 알린다. 구독자는 GetQuickSlots로 다시 그린다. 개수도 바뀔 수 있다. */
    FOnQuickSlotsReset OnReset;

private:
    UPROPERTY()
    TArray<FP1QuickSlot> QuickSlots;
};
