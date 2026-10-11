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
	virtual void		Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes, int32 errorCode) override;
    
	/** 소켓을 열어 리슨하고 AcceptEx를 서비스의 GetMaxSessionCount()개(최대 MAX_PENDING_ACCEPT_COUNT개) 걸어 둔다. */
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
	/**
	 * 새 세션을 만들어 AcceptEx를 건다. 리스너를 닫았거나, 세션을 만들지 못했거나, AcceptEx가 실패하면 걸지 않고 로그를 남긴다.
	 * 그러면 걸어 둔 AcceptEx가 하나 줄어든다. 다시 시도하지 않는다.
	 */
	void				RegisterAccept(AcceptEvent* acceptEvent);
	/**
	 * IOCP 워커 스레드에서 불린다. 세션을 연결 상태로 만든 뒤 같은 이벤트로 RegisterAccept를 다시 부른다.
	 * errorCode가 0이 아니면 받다가 실패한 접속이므로 세션을 버리고 다시 건다.
	 */
	void				ProcessAccept(AcceptEvent* acceptEvent, int32 errorCode);

	void				SetService(ServerServiceRef service) { _service = service; }


private:
	/** 미리 걸어 두는 AcceptEx 수의 상한. 접속 상한과 따로 둔다. 걸어 둔 수만큼 세션과 소켓을 미리 만든다. */
	static constexpr int32 MAX_PENDING_ACCEPT_COUNT = 64;

	SOCKET _listenSocket = INVALID_SOCKET;
	/** Close가 세운다. IOCP 워커가 읽으므로 atomic이다. 세운 뒤에는 AcceptEx를 다시 걸지 않는다. */
	atomic<bool> _closed = false;
	ServerServiceRef _service;
	vector<AcceptEvent*> _acceptEvents;
};

