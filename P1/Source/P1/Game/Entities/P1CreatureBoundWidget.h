#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "P1CreatureBoundWidget.generated.h"

class AP1Creature;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UP1CreatureBoundWidget : public UInterface
{
    GENERATED_BODY()
};

/**
 * 크리처 한 개체에 붙어 그 개체의 정보를 보여 주는 위젯이다. 예: 네임플레이트.
 * 크리처는 위젯의 구체 타입을 모르고 이 인터페이스로만 부른다. 게임 도메인이 UI/를 부르지 않기 위해서다.
 */
class P1_API IP1CreatureBoundWidget
{
    GENERATED_BODY()

public:
    /** 이 위젯이 보여 줄 크리처를 정한다. 크리처의 BeginPlay에서 한 번 부른다. */
    virtual void BindCreature(AP1Creature* Creature) = 0;
};
