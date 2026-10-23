#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Game/Chat/P1ChatLog.h"
#include "P1ChatSubsystem.generated.h"

namespace Protocol
{
    class S_CHAT;
    class S_CHAT_MESSAGE;
}

DECLARE_MULTICAST_DELEGATE(FOnChatLogChanged);

/**
 * 채팅의 송신 창구이자 받은 채팅의 보관소.
 *
 * 입력한 문장을 서버에 보내고, 서버가 알린 채팅과 거절을 채팅 줄로 보관해 구독자에게 알린다.
 * 레벨과 무관하게 살아 있어 맵을 옮겨도 채팅이 남는다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1ChatSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Send
public:
    /** 앞뒤 공백을 지운 문장이 비었거나 길이 제한을 넘으면 보내지 않고 false. */
    bool SendChat(const FString& Input);

    //~ Receive
public:
    /** 서버가 거절했으면 안내 줄을 남긴다. */
    void HandleChatResult(const Protocol::S_CHAT& ChatPkt);

    /** MyEntityId는 내 플레이어의 엔티티 번호다. 내 줄인지 가르는 데 쓴다. */
    void HandleChatMessage(const Protocol::S_CHAT_MESSAGE& ChatMessagePkt, uint64 MyEntityId);

private:
    void AddLine(FP1ChatLine Line);

    //~ Log
public:
    /** 오래된 줄이 앞에 있다. */
    const TArray<FP1ChatLine>& GetLines() const { return Log.GetLines(); }

    /** 줄을 모두 지우고 알린다. 다른 캐릭터로 들어가기 전에 부른다. */
    void ClearLog();

    /** 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다. 레벨과 함께 사라지는 구독자가 사라지기 전에 부른다. */
    void RemoveListener(const UObject* Listener);

    /** 줄이 더해지거나 지워진 뒤에 알린다. */
    FOnChatLogChanged OnChatLogChanged;

private:
    /** 보관하는 줄의 수. 화면에 보이는 줄은 채팅창이 정한다. */
    static constexpr int32 LOG_CAPACITY = 50;

    FP1ChatLog Log{ LOG_CAPACITY };
};
