#pragma once

#include "ServerCore/Network/IocpCore.h"
#include "ServerCore/Network/NetworkEvent.h"
#include "ServerCore/Network/NetAddress.h"
#include "ServerCore/Network/RecvBuffer.h"

class Service;

/*---------------------
		Session
----------------------*/

/**
 * IOCP 위에서 소켓 하나의 연결, 수신, 송신, 끊기를 맡는다. 클라이언트와 서버가 함께 쓴다.
 * I/O마다 Register*로 걸고, 완료되면 IOCP 워커 스레드에서 Process*가 처리한다.
 * 이벤트 객체를 종류마다 하나씩 재사용하므로 수신과 송신은 각각 한 번에 하나만 걸린다.
 * 콘텐츠 코드는 이 클래스(보통 PacketSession)를 상속해 On* 훅을 재정의한다.
 * 연결된 동안에는 Service의 세션 집합이, I/O가 걸린 동안에는 이벤트의 owner가 수명을 붙잡는다.
 */
class Session : public IocpObject
{
	friend class Listener;

	enum
	{
		/** 수신 버퍼 한 청크의 크기(64KB). 수신 버퍼 전체는 이 값의 10배다. */
		BUFFER_SIZE = 0x10000,
	};

public:
	Session();
	virtual ~Session();

public:
	//~ IocpObject 인터페이스 구현
	virtual HANDLE			GetHandle() override;
	virtual void			Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes, int32 errorCode) override;

public:
	//~ 통신 함수
	/** ConnectEx로 서비스 주소에 접속을 건다. ClientService에서만 쓴다. */
	bool					Connect();
	/**
	 * DisconnectEx로 연결을 끊는다. 
	 * 딱 한 번만 실행되며, 아직 보내지 못한 송신 큐의 패킷은 버려진다.
	 * 끊기가 완료되면 OnDisconnected가 불리고 서비스의 세션 집합에서 빠진다.
	 */
	void					Disconnect(const char* cause);
	/**
	 * 이미 송신 큐에 넣은 패킷을 다 보낸 뒤에 DisconnectEx로 연결을 끊는다. 이 뒤의 Send는 버린다.
	 * 상대가 받지 않으면 송신이 끝나지 않으므로, 상한이 필요하면 호출자가 Disconnect로 끊는다.
	 * cause는 끊을 때까지 살아 있어야 한다(문자열 리터럴).
	 */
	void					DisconnectAfterSend(const char* cause);
	/**
	 * 송신 큐에 직렬화된 패킷이 담긴 버퍼 참조를 추가한다.
	 * 송신이 등록된 상태라면, 큐에 쌓았다가 다음 WSASend에 모아 보낸다.
	 * sendBuffer가 nullptr이거나, 연결이 끊겼거나, DisconnectAfterSend를 부른 뒤에는 버린다.
	 * 가상인 이유는 테스트가 보낸 패킷을 기록하는 세션으로 바꿔 끼우기 때문이다. 운영 코드는 재정의하지 않는다.
	 */
	virtual void			Send(SendBufferRef sendBuffer);

public:
	//~ Session 정보 관련
	void					SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress				GetAddress() { return _netAddress; }
	SOCKET					GetSocket() { return _socket; }
	bool					IsConnected() { return _connected; }
	SessionRef				GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }
	shared_ptr<Service>		GetService() { return _service.lock(); }
	void					SetService(shared_ptr<Service> service) { _service = service; }
	
private:
	//~ 네트워크 이벤트 등록
                            /** 클라이언트에서만 쓴다. */
	bool					RegisterConnect();              
	bool					RegisterDisconnect();
	void					RegisterRecv();
                            /** 직렬화(serialized)된 패킷을 받아서 비동기 송신. */
	void					RegisterSend();

	//~ 네트워크 이벤트 완료 통지 처리
	void					ProcessConnect();						
	void					ProcessDisconnect();
	void					ProcessRecv(int32 numOfBytes);
	void					ProcessSend(int32 numOfBytes);

	/** 수신이나 송신을 걸지 못했을 때 부른다. 오류 종류와 무관하게 끊는다. 걸리지 않은 I/O는 다시 걸리지 않기 때문이다. */
	void					HandleError(int32 errorCode);

protected:
	//~ 완료 이벤트 핸들러(컨텐츠 코드에서 재정의)
	/** 아래 훅은 모두 IOCP 워커 스레드에서 불린다. 오래 걸리는 일은 JobQueue나 DBQueue로 넘긴다. */
	virtual void			OnConnected() {}
	/**
	 * 받은 바이트 중 처리한 길이를 돌려준다. 남은 바이트는 버퍼에 남아 다음 수신 때 함께 들어온다.
	 * 0보다 작거나 받은 길이보다 큰 값을 돌려주면 연결을 끊는다.
	 */
	virtual int32			OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() {}

private:
	MAKE_LOCK;
	weak_ptr<Service>		_service;
	NetAddress				_netAddress;
	SOCKET					_socket = INVALID_SOCKET;
	atomic<bool>			_connected = false;

private:
	//~ recvEvent 관련
	RecvBuffer				_recvBuffer;

	//~ sendEvent 관련
	queue<SendBufferRef>	_sendQueue;
    /** 송신 등록 상태 플래그. 동시에 여러번 송신 등록하는 걸 막기 위한 용도이다*/
	atomic<bool>			_sendRegistered = false;
	/** DisconnectAfterSend로 정한 끊기 사유. 락 안에서만 읽고 쓴다. */
	const char*				_disconnectAfterSendCause = nullptr;

private:
	//~ IocpEvent 재사용
	ConnectEvent			_connectEvent;
	DisconnectEvent			_disconnectEvent;
	RecvEvent				_recvEvent;
	SendEvent				_sendEvent;
};

/*---------------------
	  PacketSession
----------------------*/

/** 모든 패킷의 앞 4바이트. size는 헤더를 포함한 패킷 전체의 길이다. */
struct PacketHeader
{
	/** 헤더를 포함한 패킷 전체 길이(바이트). */
	uint16 size;
	/** 패킷 종류 번호. */
	uint16 id;
};

/**
 * [size][id][data] 형식의 패킷 단위로 통신하는 세션. OnRecv가 TCP 스트림을 패킷으로 잘라 OnRecvPacket을 부른다.
 * 헤더의 size가 헤더 크기보다 작으면 연결을 끊는다.
 */
class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef		GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32			OnRecv(BYTE* buffer, int32 len) final;
	/** 헤더를 포함한 완성된 패킷 하나를 받는다. IOCP 워커 스레드에서 불리며, buffer는 이 호출 안에서만 유효하다. */
	virtual void			OnRecvPacket(BYTE* buffer, int32 len) = 0;
};
