#pragma once
#include "NetAddress.h"

/*------------------
	  SocketUtil
-------------------*/

/**
 * Winsock 초기화와 소켓 함수를 모은 정적 클래스. 객체를 만들지 않는다.
 * CoreGlobal이 main보다 먼저 Init을 불러 확장 함수 포인터(ConnectEx 등)를 채운다.
 */
class SocketUtil
{
public:
	//~ 확장 함수 포인터
	static LPFN_CONNECTEX		ConnectEx;
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

private:
	static bool					alreadyInit;

public:
	/** Winsock을 초기화하고(WSAStartup) 확장 함수 포인터 3개를 얻는다. CoreGlobal 생성자에서 호출된다. */
	static bool Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	/** 비동기 I/O용(WSA_FLAG_OVERLAPPED) TCP 소켓을 만든다. */
	static SOCKET CreateSocket();

	//~ Set SockOpt
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	static bool SetReuseAddress(SOCKET socket, bool flag);
	static bool SetRecvBufferSize(SOCKET socket, int32 size);
	static bool SetSendBufferSize(SOCKET socket, int32 size);
	/** 지금은 레벨을 SOL_SOCKET으로 넘겨서 옵션이 적용되지 않는다(TD-010). */
	static bool SetTcpNoDelay(SOCKET socket, bool flag);
	/** AcceptEx로 받은 소켓에 리슨 소켓의 속성을 물려준다. 그래야 getpeername 같은 함수가 동작한다. */
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	//~ 소켓 기본 함수
	static bool Bind(SOCKET socket, NetAddress netAddr);
	static bool BindAnyAddress(SOCKET socket, uint16 port);
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	/** 소켓을 닫고 INVALID_SOCKET으로 바꾼다. */
	static void Close(SOCKET& socket);
};

/** setsockopt를 감싼 헬퍼. optVal의 크기를 옵션 길이로 넘긴다. */
template<typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}