#pragma once
#include "IocpCore.h"

class AcceptEvent;
class ServerService;

/*------------------
	Listener
-------------------*/

// 리슨 소켓을 열고 AcceptEx를 미리 여러 개 걸어 두는 객체. ServerService::Start가 만든다.
// 접속이 완료되면 미리 만들어 둔 세션을 연결 상태로 만들고, 같은 이벤트로 다음 AcceptEx를 다시 건다.
class Listener : public IocpObject
{
public:
	Listener();
	~Listener();

public:
						/* 인터페이스 구현 */
	virtual HANDLE		GetHandle() override;
	virtual void		Dispatch(class NetworkEvent* networkEvent, int32 numOfBytes = 0) override;

	// 소켓을 열어 리슨하고 AcceptEx를 서비스의 GetMaxSessionCount()개만큼 걸어 둔다.
	bool				Start();

						/* 수신 관련 */
	// 새 세션을 만들어 AcceptEx를 건다. 실패하면 같은 이벤트로 다시 시도한다.
	void				RegisterAccept(AcceptEvent* acceptEvent);
	// IOCP 워커 스레드에서 불린다. 세션을 연결 상태로 만든 뒤 같은 이벤트로 RegisterAccept를 다시 부른다.
	void				ProcessAccept(AcceptEvent* acceptEvent);

	void				SetService(ServerServiceRef service) { _service = service; }

private:
	bool				Listen();
	bool				Accept();

private:
	SOCKET _listenSocket = INVALID_SOCKET;
	ServerServiceRef _service;
	vector<AcceptEvent*> _acceptEvents;
};

