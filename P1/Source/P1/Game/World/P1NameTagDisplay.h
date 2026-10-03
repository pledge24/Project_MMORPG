#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "P1NameTagDisplay.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UP1NameTagDisplay : public UInterface
{
    GENERATED_BODY()
};

/**
 * 레벨에 놓인 액터 위에 이름표를 띄우는 위젯이다. 예: 포털 이름표.
 * 액터는 위젯의 구체 타입을 모르고 이 인터페이스로만 부른다. Game/이 UI/를 부르지 않기 위해서다.
 */
class P1_API IP1NameTagDisplay
{
    GENERATED_BODY()

public:
    /** 이름표의 문구와 글자 색을 입힌다. 에디터에서 액터를 배치하거나 고칠 때도 불린다. */
    virtual void SetNameTag(const FText& Text, const FLinearColor& Color) = 0;
};
