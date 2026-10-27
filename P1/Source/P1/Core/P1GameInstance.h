#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "P1GameInstance.generated.h"

/**
 * 클라이언트가 켜져 있는 동안 살아 있는 게임 인스턴스.
 *
 * 게임 인스턴스 블루프린트가 부모로 삼는 C++ 기반이 된다.
 */
UCLASS()
class P1_API UP1GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UP1GameInstance();

    //~ Begin UGameInstance Interface
public:
    virtual void Init() override;
    //~ End UGameInstance Interface
};
