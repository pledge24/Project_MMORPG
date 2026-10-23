#include "Core/pch.h"
#include "Network/ChatRequests.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Utils/EncodingConverter.h"

optional<Protocol::ChatResult> ChatRequests::Validate(const string& message)
{
    // 한글 메시지만 짧아지지 않도록 바이트가 아니라 UTF-16 글자로 센다.
    const wstring wideMessage = EncodingConverter::StringToWString(message);
    if (wideMessage.empty() || wideMessage.size() > MAX_MESSAGE_LENGTH)
        return Protocol::CHAT_RESULT_INVALID_MESSAGE;

    return nullopt;
}

void ChatRequests::HandleChat(Room& room, const PlayerRef& player, const Protocol::C_CHAT& pkt)
{
    // 맵 로드를 기다리는 플레이어는 소속 룸에 아직 없다. 그 룸에 중계하면 들어가지 않은 룸에 말이 남는다.
    if (room.Contains(player->GetEntityId()) == false)
        return;

    auto session = player->GetSession();
    if (session == nullptr)
        return;

    Protocol::S_CHAT chatPkt;
    const optional<Protocol::ChatResult> rejection = Validate(pkt.message());
    chatPkt.set_result(rejection.value_or(Protocol::CHAT_RESULT_SUCCESS));
    SendPacket(session, chatPkt);

    if (rejection.has_value())
        return;

    Protocol::S_CHAT_MESSAGE chatMessagePkt;
    chatMessagePkt.set_entity_id(player->GetEntityId());
    chatMessagePkt.set_message(pkt.message());
    room.Broadcast(ServerPacketHandler::MakeSerializedPacket(chatMessagePkt));
}
