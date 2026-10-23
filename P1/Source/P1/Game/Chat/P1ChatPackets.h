#pragma once

#include "CoreMinimal.h"
#include "Network/Chat.pb.h"
#include "Game/Chat/P1ChatLine.h"
#include "Game/Chat/P1ChatLimits.h"

/**
 * 채팅 줄과 채팅 패킷 사이의 변환.
 *
 * 채팅의 Protocol 타입은 이 변환에서만 다룬다. 받은 메시지를 채팅 줄로 읽고, 입력한 문장으로 채팅 요청을 만든다.
 */
namespace P1ChatPackets
{
    /** MyEntityId와 엔티티 번호가 같으면 내 줄이 된다. 이름을 받지 못했으면 이름이 빈 줄이 된다. */
    FP1ChatLine FromProto(const Protocol::S_CHAT_MESSAGE& ChatMessagePkt, uint64 MyEntityId);

    /** 앞뒤 공백을 지운 문장으로 요청을 만든다. 남은 문장이 비었거나 길이 제한을 넘으면 만들지 않는다. */
    TOptional<Protocol::C_CHAT> ToRequest(const FString& Input);
}
