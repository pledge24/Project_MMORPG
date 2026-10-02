#include "Game/World/P1Portal.h"
#include "Network/P1PacketSender.h"

void AP1Portal::SendEnterRoomPacket()
{
    if (PortalId == 0)
        return;

    Protocol::C_ENTER_ROOM EnterRoomPkt; 
    {
        EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_SAME_MAP_TRANSFER);
        EnterRoomPkt.set_portal_id(PortalId);

        FP1PacketSender::Send(this, EnterRoomPkt);
    }
}


