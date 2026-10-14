#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Enum.pb.h"
#include "P1GameInstance.generated.h"

/**
 * 클라이언트가 켜져 있는 동안 살아 있는 게임 인스턴스.
 *
 * 맵 사이의 레벨 전환을 맡고, 연결이 끊기면 그 사유를 로그인 화면까지 전한다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
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

    //~ Level Transition
private:
    void OpenInGameMap();

    //~ Connection Loss
public:
    /** 로그인 화면에 보여 줄 문구를 꺼내고 비운다. 연결이 끊겨 돌아온 것이 아니면 빈 문자열. */
    FString ConsumeLoginNotice();

private:
    void HandleConnectionLost(Protocol::LeaveReason Reason);

    /** 로그인 맵을 열고, 열린 로그인 화면에 Notice를 보여 준다. */
    void ReturnToLogin(const FString& Notice);

    /** 로그인 맵이 열린 뒤 로그인 화면에 보여 줄 문구. 레벨 전환을 건너 들고 간다. */
    FString PendingLoginNotice;
};
