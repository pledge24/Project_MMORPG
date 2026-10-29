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
 * NPC 상호작용이 화면을 다루는 창구.
 *
 * 상호작용 컴포넌트가 화면의 구체 타입을 모른 채 상호작용 표시와 상점 창을 다루게 한다.
 * 게임 도메인이 UI/와 Core/를 부르지 않기 위해서다.
 */
class P1_API IP1ShopScreen
{
    GENERATED_BODY()

public:
    /** 조준한 NPC로 할 수 있는 일을 화면에 띄운다. 빈 문구면 내린다. */
    virtual void ShowInteractionPrompt(const FText& Prompt) = 0;

    /** 다른 창을 모두 내리고 NpcEntityId NPC의 ShopId 상점과 인벤토리를 연다. 상점이 열린 동안은 캐릭터 조작과 카메라 회전을 막는다. */
    virtual void OpenShopWindow(int64 NpcEntityId, int32 ShopId) = 0;

    /** 상점과 인벤토리를 닫고 조작을 돌려준다. 상점이 닫혀 있으면 아무것도 하지 않는다. */
    virtual void CloseShopWindow() = 0;
};
