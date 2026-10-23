#include "Game/Chat/P1ChatSubsystem.h"
#include "Game/Chat/P1ChatLimits.h"
#include "Game/Chat/P1ChatPackets.h"
#include "Network/P1PacketSender.h"
#include "Utils/LogCategory.h"

bool UP1ChatSubsystem::SendChat(const FString& Input)
{
    TOptional<Protocol::C_CHAT> ChatPkt = P1ChatPackets::ToRequest(Input);
    if (ChatPkt.IsSet() == false)
        return false;

    FP1PacketSender::Send(GetGameInstance(), ChatPkt.GetValue());
    return true;
}

void UP1ChatSubsystem::HandleChatResult(const Protocol::S_CHAT& ChatPkt)
{
    if (ChatPkt.result() == Protocol::CHAT_RESULT_SUCCESS)
        return;

    UE_LOG(LogP1Chat, Warning, TEXT("채팅이 거절됐습니다: %s"), UTF8_TO_TCHAR(Protocol::ChatResult_Name(ChatPkt.result()).c_str()));

    // 결과 종류는 하나뿐이고, 입력칸이 길이를 막으므로 여기 오는 것은 서버만 거절한 경우다.
    FP1ChatLine Notice;
    Notice.Kind = EP1ChatLineKind::System;
    Notice.Message = FString::Printf(TEXT("메시지를 보내지 못했습니다. 비어 있거나 %d자를 넘었습니다."), P1ChatLimits::MAX_MESSAGE_LENGTH);
    AddLine(MoveTemp(Notice));
}

void UP1ChatSubsystem::HandleChatMessage(const Protocol::S_CHAT_MESSAGE& ChatMessagePkt, uint64 MyEntityId)
{
    AddLine(P1ChatPackets::FromProto(ChatMessagePkt, MyEntityId));
}

void UP1ChatSubsystem::ClearLog()
{
    Log.Clear();
    OnChatLogChanged.Broadcast();
}

void UP1ChatSubsystem::RemoveListener(const UObject* Listener)
{
    OnChatLogChanged.RemoveAll(Listener);
}

void UP1ChatSubsystem::AddLine(FP1ChatLine Line)
{
    Log.Add(MoveTemp(Line));
    OnChatLogChanged.Broadcast();
}
