#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1NetworkSettings.generated.h"

/**
 * 클라이언트가 접속할 서버 주소 설정.
 *
 * 인증 서버의 주소를 담고, 에디터의 프로젝트 설정 「P1 Network」에서 바꿀 수 있게 한다.
 * 게임 서버의 주소는 월드 선택이 알려 주므로 여기에 없다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Network"))
class P1_API UP1NetworkSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 인증 서버의 IP. */
    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    FString AuthServerIp = TEXT("127.0.0.1");

    /** 인증 서버의 포트. */
    UPROPERTY(Config, EditAnywhere, Category = "Auth Server")
    int32 AuthServerPort = 5000;
};
