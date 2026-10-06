#pragma once

/*----------------
	IocpObject
-----------------*/

/**
 * IOCP 완료 통지를 받아 처리하는 객체의 인터페이스. Listener와 Session이 구현한다.
 * I/O를 걸 때 shared_from_this로 NetworkEvent::owner에 자신을 걸므로, 반드시 shared_ptr로 만든다.
 */
class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() = 0;
	/** IocpCore::Dispatch가 IOCP 워커 스레드에서 호출한다. 실패한 I/O도 들어오며, 그때 numOfBytes는 대개 0이다. */
	virtual void Dispatch(class NetworkEvent* iocpEvent, int32 numOfBytes = 0) = 0;
};

/*-----------------
	  IocpCore
------------------*/

/**
 * IOCP 소켓 모델 클래스. CICP로 받아온 IOCP 핸들 하나를 소유한다.
 * Service가 shared_ptr로 참조하며, 워커 스레드들이 Dispatch를 반복해서 부른다.
 * 완료 키(Completion Key)는 쓰지 않는다. 완료된 NetworkEvent의 owner(Listener, Session)가 완료 처리 역할을 대신한다.
 */
class IocpCore
{
public:
	IocpCore();
	~IocpCore();

public:
	/** 완료 키 0으로 소켓을 이 IOCP에 연결한다. */
	bool		RegisterSocket(SOCKET socket);
	/** 완료 패킷 하나를 꺼내 owner의 Dispatch로 넘긴다. timeoutMs(ms) 안에 완료가 없으면 false를 돌려준다. */
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocpHandle;
};

