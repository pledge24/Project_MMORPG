#pragma once

/** NetworkEvent가 어떤 비동기 I/O의 완료인지 나타낸다. owner가 이 값으로 처리 함수를 고른다. */
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

/**
 * OVERLAPPED 포인터를 확장한 클래스.
 * 비동기 I/O를 요청할 때 넘겨주면 GQCS 함수가 되돌려준다.
 * 되돌려받은 해당 클래스를 통해 어떤 종류의 I/O가 끝났는지(eventType)와 누가 처리할지(owner)를 알아낸다.
 * Session과 Listener는 이 객체를 멤버로 만들어 두고 재사용한다.
 */
class NetworkEvent : public WSAOVERLAPPED
{
public:
	NetworkEvent(EventType type);

	/** OVERLAPPED 필드를 0으로 되돌린다. 같은 객체로 I/O를 다시 걸기 전에(Register*) 반드시 부른다. */
	void			Init();

public:
	EventType		eventType;
	/** 이 I/O의 완료를 처리할 객체. I/O가 걸려 있는 동안 그 객체를 살려 두고, 완료 처리(Process*)의 첫 줄에서 비운다. */
	IocpObjectRef	owner;
};

/*-----------------
	ConnectEvent
-------------------*/

/** ConnectEx로 건 접속의 완료. 클라이언트 세션만 쓴다. */
class ConnectEvent : public NetworkEvent
{
public:
	ConnectEvent() : NetworkEvent(EventType::Connect) {}
};

/*-------------------
	DisconnectEvent
--------------------*/

/** DisconnectEx로 건 연결 끊기의 완료. */
class DisconnectEvent : public NetworkEvent
{
public:
	DisconnectEvent() : NetworkEvent(EventType::Disconnect) {}
};

/*-------------------
	AcceptEvent
--------------------*/

/** AcceptEx로 건 접속 수락의 완료. */
class AcceptEvent : public NetworkEvent
{
public:
	AcceptEvent() : NetworkEvent(EventType::Accept) {}
	/** 이 AcceptEx로 접속을 받을 세션. 접속이 완료될 때까지 세션을 붙잡는다. */
	SessionRef		session = nullptr;
};

/*--------------
	RecvEvent
----------------*/

/** WSARecv로 건 수신의 완료. */
class RecvEvent : public NetworkEvent
{
public:
	RecvEvent() : NetworkEvent(EventType::Recv) {}
};

/*--------------
	SendEvent
----------------*/

/** WSASend로 건 송신의 완료. */
class SendEvent : public NetworkEvent
{
public:
	SendEvent() : NetworkEvent(EventType::Send) {}

	/** 이번 WSASend로 보내는 버퍼들. 송신이 끝날 때까지 버퍼를 살려 두고, ProcessSend가 비운다. */
	vector<SendBufferRef> sendBuffers;
};

