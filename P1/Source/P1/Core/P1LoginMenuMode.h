#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P1LoginMenuMode.generated.h"

/**
 * 로그인 맵의 게임 모드.
 *
 * 로그인 맵에서 쓰는 블루프린트 게임 모드가 부모로 삼는 C++ 기반이 된다.
 */
UCLASS()
class P1_API AP1LoginMenuMode : public AGameModeBase
{
    GENERATED_BODY()
};
