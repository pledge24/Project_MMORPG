#pragma once

#include "IocpCore.h"
#include "NetworkEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

/*---------------------
		Session
----------------------*/

// 비동기 방식(등록-처리)의 세션
// 클라이언트 / 서버 공용
// 현재: 양쪽 전부 IOCP 사용.
class Session : public IocpObject
{
	friend class Listener;

	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
							/* 인터페이스 구현(외부 사용) */
	virtual HANDLE			GetHandle() override;
	virtual void			Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes = 0) override;

public:
							/* 통신 함수(외부 사용) */
	bool					Connect();								// Only-use Client
	void					Disconnect(const char* cause);
	void					Send(SendBufferRef sendBuffer);

public:
							/* Session 정보(외부 사용) */
	void					SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress				GetAddress() { return _netAddress; }
	SOCKET					GetSocket() { return _socket; }
	bool					IsConnected() { return _connected; }
	SessionRef				GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }
	shared_ptr<Service>		GetService() { return _service.lock(); }
	void					SetService(shared_ptr<Service> service) { _service = service; }
	
private:
							/* 네트워크 이벤트 등록 */
	bool					RegisterConnect();						// Only-use Client
	bool					RegisterDisconnect();
	void					RegisterRecv();
	void					RegisterSend();

							/* 완료 통지 관련 */
	void					ProcessConnect();						// Only-use Client
	void					ProcessDisconnect();
	void					ProcessRecv(int32 numOfBytes);
	void					ProcessSend(int32 numOfBytes);

	void					HandleError(int32 errorCode);

protected:
							/* 컨텐츠 코드에서 재정의 */
	virtual void			OnConnected() {}
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
							/* recvEvent 관련 */
	RecvBuffer				_recvBuffer;

							/* sendEvent 관련 */
	queue<SendBufferRef>	_sendQueue;
	atomic<bool>			_sendRegistered = false;

private:
							/* IocpEvent 재사용 */
	ConnectEvent			_connectEvent;
	DisconnectEvent			_disconnectEvent;
	RecvEvent				_recvEvent;
	SendEvent				_sendEvent;
};

/*---------------------
	  PacketSession
----------------------*/

struct PacketHeader
{
	uint16 size; // 2 BYTE
	uint16 id;	 // 2 BYTE
};

// 패킷 단위로 통신하는 세션
class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionRef		GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32			OnRecv(BYTE* buffer, int32 len) final;
	virtual void			OnRecvPacket(BYTE* buffer, int32 len) = 0;
};
