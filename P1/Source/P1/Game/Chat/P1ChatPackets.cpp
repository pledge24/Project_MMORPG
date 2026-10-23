#include "Game/Chat/P1ChatPackets.h"

FP1ChatLine P1ChatPackets::FromProto(const Protocol::S_CHAT_MESSAGE& ChatMessagePkt, uint64 MyEntityId)
{
    FP1ChatLine Line;
    Line.Kind = (MyEntityId != 0 && static_cast<uint64>(ChatMessagePkt.entity_id()) == MyEntityId) ? EP1ChatLineKind::Mine : EP1ChatLineKind::Other;
    Line.Name = UTF8_TO_TCHAR(ChatMessagePkt.name().c_str());
    Line.Message = UTF8_TO_TCHAR(ChatMessagePkt.message().c_str());
    return Line;
}

TOptional<Protocol::C_CHAT> P1ChatPackets::ToRequest(const FString& Input)
{
    // 서버도 길이를 UTF-16 글자로 센다. FString의 길이가 UTF-16 글자 수라 그대로 비교한다.
    const FString Message = Input.TrimStartAndEnd();
    if (Message.IsEmpty() || Message.Len() > P1ChatLimits::MAX_MESSAGE_LENGTH)
        return {};

    Protocol::C_CHAT ChatPkt;
    ChatPkt.set_message(TCHAR_TO_UTF8(*Message));
    return ChatPkt;
}
