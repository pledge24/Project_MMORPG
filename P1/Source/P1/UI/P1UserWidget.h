#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1UserWidget.generated.h"

class UP1GameInstance;

/**
 * 프로젝트 위젯의 공용 베이스.
 *
 * 모든 위젯이 UUserWidget 대신 이 클래스를 상속하고, 위젯이 공통으로 쓰는 소유 컨텍스트를 얻는다.
 * Tick을 켜지 않는다. 값 갱신은 델리게이트 구독으로 한다.
 */
UCLASS()
class P1_API UP1UserWidget : public UUserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    /** 내 플레이어 데이터의 델리게이트에서 이 위젯을 모두 뗀다. 오버라이드하면 Super를 부른다. */
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Owning Context
public:
    /** 이 위젯을 만든 플레이어 컨트롤러. 소유자 없이 만든 위젯이면 nullptr. */
    APlayerController* GetP1PlayerController() const;

    /** 소유 컨트롤러가 없거나 T가 아니면 nullptr. */
    template <typename T>
    T* GetP1PlayerController() const
    {
        return Cast<T>(GetP1PlayerController());
    }

    /** 월드가 없거나 게임 인스턴스가 UP1GameInstance가 아니면 nullptr. */
    UP1GameInstance* GetP1GameInstance() const;
};
