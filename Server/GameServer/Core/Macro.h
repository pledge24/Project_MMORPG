#pragma once

/** GameServer 전역 매크로를 정의한다. pch.h에서 ServerPacketHandler.h 뒤에 포함한다. */

//~ Packet
#define SEND_PACKET(pkt)													    \
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(pkt);	\
	session->Send(sendBuffer);

#define SEND_PACKET_USING_THIS_SESSION(session, pkt)										        \
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(pkt);	\
	session->Send(sendBuffer);
