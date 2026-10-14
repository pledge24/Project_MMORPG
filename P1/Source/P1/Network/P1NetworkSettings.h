#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1NetworkSettings.generated.h"

/**
 * 클라이언트가 접속할 서버 주소 설정.
 *
 * 게임 서버와 인증 서버의 주소를 담고, 에디터의 프로젝트 설정 「P1 Network」에서 바꿀 수 있게 한다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Network"))
class P1_API UP1NetworkSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 게임 서버의 IP. */
    UPROPERTY(Config, EditAnywhere, Category = "Game Server")
    FString GameServerIp = TEXT("127.0.0.1");

    /** 게임 서버의 포트. */
    UPROPERTY(Config, EditAnywhere, Category = "Game Server")
    int32 GameServerPort = 7777;

    /** 인증 서버의 IP. */
    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    FString AuthServerIp = TEXT("127.0.0.1");

    /** 인증 서버의 포트. */
    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    int32 AuthServerPort = 5000;
};
