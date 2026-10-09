#pragma once
#include "ServerCore/Network/IocpCore.h"

class AcceptEvent;
class ServerService;

/*------------------
	  Listener
-------------------*/

/**
 * 리슨 소켓을 여는 리스너 클래스. AcceptEx를 미리 여러 개 등록해두는 것이 특징.
 * 접속이 완료되면 미리 만들어 둔 세션을 연결 상태로 만들고, 같은 이벤트로 다음 AcceptEx를 다시 건다.
 * ServerService::Start가 사용한다. 
 */
class Listener : public IocpObject
{
public:
	Listener();
	~Listener();

public:
	//~ 인터페이스 구현
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes = 0) override;
    
	/** 소켓을 열어 리슨하고 AcceptEx를 서비스의 GetMaxSessionCount()개만큼 걸어 둔다. */
	bool				Start();
	/**
	 * 리슨 소켓을 닫아 새 접속을 더 받지 않는다. 걸어 둔 AcceptEx는 실패로 완료되고 다시 걸리지 않는다.
	 * 이미 접속한 세션은 그대로 둔다. 여러 번 불러도 한 번만 닫는다.
	 */
	void				Close();
    
private:
	bool				Listen();
    
public:
	//~ 수신 관련
	/** 새 세션을 만들어 AcceptEx를 건다. 실패하면 같은 이벤트로 다시 시도한다. */
	void				RegisterAccept(AcceptEvent* acceptEvent);
	/** IOCP 워커 스레드에서 불린다. 세션을 연결 상태로 만든 뒤 같은 이벤트로 RegisterAccept를 다시 부른다. */
	void				ProcessAccept(AcceptEvent* acceptEvent);

	void				SetService(ServerServiceRef service) { _service = service; }


private:
	SOCKET _listenSocket = INVALID_SOCKET;
	/** Close가 세운다. IOCP 워커가 읽으므로 atomic이다. 세운 뒤에는 AcceptEx를 다시 걸지 않는다. */
	atomic<bool> _closed = false;
	ServerServiceRef _service;
	vector<AcceptEvent*> _acceptEvents;
};

