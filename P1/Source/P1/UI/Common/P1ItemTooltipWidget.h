#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Data/P1ItemData.h"
#include "P1ItemTooltipWidget.generated.h"

class UImage;
class UTexture2D;
class UTextBlock;
class UVerticalBox;

/**
 * 아이템 툴팁 위젯.
 *
 * 아이템 하나의 아이콘, 이름, 설명과 종류·능력치 같은 세부 정보를 보여 준다.
 */
UCLASS()
class P1_API UP1ItemTooltipWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Item Display
public:
    /** 아이콘은 부르는 쪽이 에셋 테이블에서 찾아 넘긴다. nullptr이면 아이콘을 바꾸지 않는다. */
    void Init(const FP1ItemData& Item, UTexture2D* Icon);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemDescriptionText;

    //~ Item Details
public:
    /** 세부 정보를 한 줄씩 적을 텍스트 블록들. 담긴 줄 수보다 많은 정보는 보이지 않는다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> VB_ItemInfo;

private:
    /**
     * VB_ItemInfo의 ChildIdx번째 줄을 채우고 ChildIdx를 다음 줄로 옮긴다.
     * 값이 비었거나 0 이하이면 아무것도 적지 않는다. 아래 두 함수도 같다.
     */
    void SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent = false);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent = false);
};
