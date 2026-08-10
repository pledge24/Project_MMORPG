#include "pch.h"
#include "NetworkEvent.h"

/*----------------
	NetworkEvent
-----------------*/

NetworkEvent::NetworkEvent(EventType type) : eventType(type)
{
	Init();
}

void NetworkEvent::Init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}
