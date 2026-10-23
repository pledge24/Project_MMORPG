#include "Core/pch.h"
#include "Network/SessionPhase.h"

namespace
{
    using PhaseMask = uint8;

    constexpr PhaseMask Only(SessionPhase phase)
    {
        return static_cast<PhaseMask>(1u << static_cast<uint8>(phase));
    }

    constexpr PhaseMask ANY_PHASE = Only(SessionPhase::BEFORE_LOBBY) | Only(SessionPhase::CONNECTING_LOBBY) | Only(SessionPhase::LOBBY)
        | Only(SessionPhase::ENTERING_GAME) | Only(SessionPhase::IN_GAME);

    struct PacketPhases
    {
        uint16 packetId;
        PhaseMask phases;
    };

    constexpr PacketPhases PACKET_PHASES[] =
    {
        { PKT_C_PING, ANY_PHASE },
        // 클라이언트가 게임을 끌 때 단계와 상관없이 보낸다.
        { PKT_C_LEAVE_GAME, ANY_PHASE },

        // 로비 접속은 한 번만 받는다. 입장한 세션이 다른 계정의 인증 티켓을 내면 세션의 계정 번호만 바뀐다.
        { PKT_C_CONNECT_LOBBY, Only(SessionPhase::BEFORE_LOBBY) },

        // 게임에 입장한 세션이 플레이 중인 캐릭터를 지우면 플레이어는 룸에 남고 삭제된 캐릭터의 진행이 계속 저장된다.
        { PKT_C_CREATE_CHARACTER, Only(SessionPhase::LOBBY) },
        { PKT_C_DELETE_CHARACTER, Only(SessionPhase::LOBBY) },
        { PKT_C_ENTER_GAME, Only(SessionPhase::LOBBY) },

        { PKT_C_MAP_LOAD_COMPLETE, Only(SessionPhase::IN_GAME) },
        { PKT_C_ENTER_MAP, Only(SessionPhase::IN_GAME) },
        { PKT_C_ENTER_ROOM, Only(SessionPhase::IN_GAME) },
        { PKT_C_MOVE, Only(SessionPhase::IN_GAME) },
        { PKT_C_NORMAL_ATTACK, Only(SessionPhase::IN_GAME) },
        { PKT_C_BUY_ITEM, Only(SessionPhase::IN_GAME) },
        { PKT_C_SELL_ITEM, Only(SessionPhase::IN_GAME) },
        { PKT_C_EQUIP_GEAR, Only(SessionPhase::IN_GAME) },
        { PKT_C_UNEQUIP_GEAR, Only(SessionPhase::IN_GAME) },
        { PKT_C_USE_ITEM, Only(SessionPhase::IN_GAME) },
        { PKT_C_SET_QUICK_SLOT, Only(SessionPhase::IN_GAME) },
        { PKT_C_USE_QUICK_SLOT, Only(SessionPhase::IN_GAME) },
        { PKT_C_RESPAWN, Only(SessionPhase::IN_GAME) },
        { PKT_C_CHAT, Only(SessionPhase::IN_GAME) },
    };
}

bool SessionPhaseTable::Accepts(uint16 packetId, SessionPhase phase)
{
    for (const PacketPhases& entry : PACKET_PHASES)
    {
        if (entry.packetId == packetId)
            return (entry.phases & Only(phase)) != 0;
    }

    return false;
}
