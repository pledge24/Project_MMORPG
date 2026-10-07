#pragma once
#include "ServerCore/Network/NetAddress.h"

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

	//~ 소켓 옵션 설정 관련
    /**
     * SetLinger: 소켓을 닫을때(closesocket) 송신 버퍼에 남은 데이터 처리 방식을 설정한다.
     * - onoff = 0이면 기본 동작. 남은 데이터를 백그라운드에서 전송하고 정상 종료(FIN)한다.
     * - onoff = 1, linger = 0이면 남은 데이터를 버리고 RST를 보내 연결을 즉시 끊는다.
     * - onoff = 1, linger > 0이면 최대 linger초만큼 대기 후 연결을 끊는다.
     * SetReuseAddress: 이미 사용 중인 주소와 포트에 bind 할 수 있도록 허용한다.
     * SetRecvBufferSize: 운영체제 커널이 소켓마다 관리하는 "수신" 버퍼의 크기를 설정한다.
     * SetSendBufferSize: 운영체제 커널이 소켓마다 관리하는 "송신" 버퍼의 크기를 설정한다.
     * SetTcpNoDelay: Nagle 알고리즘을 비활성화하여 작은 패킷을 지연 없이 전송하도록 한다.
     * SetUpdateAcceptSocket: AcceptEx로 받은 클라 소켓에 리슨 소켓의 속성을 물려준다.
     * -> 그래야 getpeername 같은 함수가 동작한다.
     */
    static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	static bool SetReuseAddress(SOCKET socket, bool flag);
	static bool SetRecvBufferSize(SOCKET socket, int32 size);
	static bool SetSendBufferSize(SOCKET socket, int32 size);
	static bool SetTcpNoDelay(SOCKET socket, bool flag);
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