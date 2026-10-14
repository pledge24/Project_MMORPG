// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P1GameModeBase.generated.h"

/**
 * 이 프로젝트의 기본 게임 모드.
 *
 * 블루프린트 게임 모드가 부모로 삼는 C++ 기반이 된다.
 */
UCLASS()
class P1_API AP1GameModeBase : public AGameModeBase
{
    GENERATED_BODY()

    //~ Begin AGameModeBase Interface
private:
    virtual void StartPlay() override;
    //~ End AGameModeBase Interface

    //~ Begin AActor Interface
private:
    virtual void BeginPlay() override;
    //~ End AActor Interface
};
