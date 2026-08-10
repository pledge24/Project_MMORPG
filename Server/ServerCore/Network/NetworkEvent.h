#pragma once

enum class EventType 
{ 
	Connect, 
	Disconnect, 
	Accept, 
	Recv, 
	Send
};

/*-----------------
	NetworkEvent
-------------------*/

class NetworkEvent : public WSAOVERLAPPED
{
public:
	NetworkEvent(EventType type);

	void			Init();

public:
	EventType		eventType;
	IocpObjectRef	owner;
};

/*-----------------
	ConnectEvent
-------------------*/

class ConnectEvent : public NetworkEvent
{
public:
	ConnectEvent() : NetworkEvent(EventType::Connect) {}
};

/*-------------------
	DisconnectEvent
--------------------*/

class DisconnectEvent : public NetworkEvent
{
public:
	DisconnectEvent() : NetworkEvent(EventType::Disconnect) {}
};

/*-------------------
	AcceptEvent
--------------------*/

class AcceptEvent : public NetworkEvent
{
public:
	AcceptEvent() : NetworkEvent(EventType::Accept) {}
	SessionRef		session = nullptr;
};

/*--------------
	RecvEvent
----------------*/

class RecvEvent : public NetworkEvent
{
public:
	RecvEvent() : NetworkEvent(EventType::Recv) {}
};

/*--------------
	SendEvent
----------------*/

class SendEvent : public NetworkEvent
{
public:
	SendEvent() : NetworkEvent(EventType::Send) {}

	vector<SendBufferRef> sendBuffers;
};

