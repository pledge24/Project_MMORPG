#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "P1ShopScreen.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UP1ShopScreen : public UInterface
{
    GENERATED_BODY()
};

/**
 * 상점이 상점 창과 경고 문구를 다루는 창구다. 인게임 컨트롤러가 구현해 화면 서브시스템에 넘긴다.
 * 상점은 화면의 구체 타입을 모르고 이 인터페이스로만 부른다. 게임 도메인이 UI/와 Core/를 부르지 않기 위해서다.
 */
class P1_API IP1ShopScreen
{
    GENERATED_BODY()

public:
    /** 내 플레이어가 상점 범위에 들어왔고 상점 창을 열 수 있을 때 부른다. */
    virtual void OpenShopWindow() = 0;

    /** 상점 범위에서 겹침이 끝날 때 부른다. 창이 닫혀 있으면 아무것도 하지 않는다. */
    virtual void CloseShopWindow() = 0;

    /** 상점 창을 열 수 없는 이유를 플레이어에게 알릴 때 부른다. */
    virtual void ShowShopWarning(const FText& Message) = 0;
};
