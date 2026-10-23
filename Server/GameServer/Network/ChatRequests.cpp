#include "Core/pch.h"
#include "Network/ChatRequests.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Game/Room/RoomManager.h"
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

    // 채팅은 룸이 아니라 게임 서버 단위로 들린다. 받는 쪽은 다른 룸의 플레이어를 엔티티로 모르므로 이름을 싣는다.
    Protocol::S_CHAT_MESSAGE chatMessagePkt;
    chatMessagePkt.set_entity_id(player->GetEntityId());
    chatMessagePkt.set_message(pkt.message());
    chatMessagePkt.set_name(player->GetPlayerInfo().name());
    const SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(chatMessagePkt);

    // 룸의 플레이어 목록은 그 룸의 큐만 읽는다. 말한 플레이어의 룸도 같은 경로로 보내 룸마다 도착 순서를 큐가 정하게 한다.
    for (const RoomRef& targetRoom : GRoomManager->GetAllRooms())
        targetRoom->DoAsync([targetRoom, sendBuffer]() { targetRoom->Broadcast(sendBuffer); });
}
