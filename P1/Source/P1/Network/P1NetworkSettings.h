#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1NetworkSettings.generated.h"

/**
 * 클라이언트가 붙는 서버 주소다. 기본값은 Config/DefaultGame.ini에 있고,
 * 에디터의 프로젝트 설정 「P1 Network」에서 바꿀 수 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Network"))
class P1_API UP1NetworkSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Game Server")
    FString GameServerIp = TEXT("127.0.0.1");

    UPROPERTY(Config, EditAnywhere, Category = "Game Server")
    int32 GameServerPort = 7777;

    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    FString AuthServerIp = TEXT("127.0.0.1");

    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    int32 AuthServerPort = 5000;
};
