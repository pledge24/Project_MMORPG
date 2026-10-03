#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Enum.pb.h"
#include "P1GameInstance.generated.h"

/** 레벨 전환을 맡는다. 인게임 맵을 열고, 연결이 끊기면 사유 문구와 함께 로그인 맵으로 돌아간다. */
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
    /** 입장 성공과 맵 입장에서 부른다. 게임 스레드 전용. */
    void OpenInGameMap();

    //~ Connection Loss
public:
    /** 로그인 화면이 한 번 꺼내 보여 준다. 꺼내면 비워진다. 연결이 끊겨 돌아온 것이 아니면 비어 있다. 게임 스레드 전용. */
    FString ConsumeLoginNotice();

private:
    /** 연결 서브시스템이 끊김을 알렸다. 사유에 맞는 문구로 로그인 화면에 돌아간다. */
    void HandleConnectionLost(Protocol::LeaveReason Reason);

    /** 로그인 맵을 연다. 연결은 연결 서브시스템이 이미 닫았다. 사용자가 게임을 끄는 경로(연결 서브시스템의 Deinitialize)는 이 경로를 타지 않는다. 게임 스레드 전용. */
    void ReturnToLogin(const FString& Notice);

    /** 로그인 맵이 열린 뒤 로그인 화면에 보여 줄 문구다. 게임 인스턴스가 레벨 전환을 건너 들고 간다. */
    FString PendingLoginNotice;
};
