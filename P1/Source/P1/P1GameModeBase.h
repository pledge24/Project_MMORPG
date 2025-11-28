// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "P1GameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class P1_API AP1GameModeBase : public AGameModeBase
{
	GENERATED_BODY()

    virtual void StartPlay() override;

    virtual void BeginPlay() override;
};
