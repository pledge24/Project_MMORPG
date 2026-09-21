#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1UserWidget.generated.h"

class UP1GameInstance;

/**
 * 모든 위젯의 공용 베이스다. UUserWidget을 직접 상속하지 않는다.
 * 이 클래스에서 Tick을 켜지 않는다. 값 갱신은 델리게이트 구독으로 한다.
 */
UCLASS()
class P1_API UP1UserWidget : public UUserWidget
{
    GENERATED_BODY()

    //~ Owning Context
public:
    /** 이 위젯을 만든 플레이어 컨트롤러다. 소유자 없이 만든 위젯에서는 nullptr을 돌려준다. */
    APlayerController* GetP1PlayerController() const;

    /** 소유 컨트롤러가 T가 아니면 nullptr을 돌려준다. */
    template <typename T>
    T* GetP1PlayerController() const
    {
        return Cast<T>(GetP1PlayerController());
    }

    /** 월드가 없거나 게임 인스턴스가 UP1GameInstance가 아니면 nullptr을 돌려준다. */
    UP1GameInstance* GetP1GameInstance() const;
};
