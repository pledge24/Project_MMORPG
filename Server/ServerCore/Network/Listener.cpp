#include "ServerCore/Core/pch.h"
#include "ServerCore/Network/Listener.h"
#include "ServerCore/Network/NetAddress.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/SocketUtil.h"
#include "ServerCore/Network/NetworkEvent.h"
#include "ServerCore/Network/Session.h"

Listener::Listener()
{
}

Listener::~Listener()
{
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_listenSocket);
}

void Listener::Dispatch(NetworkEvent* networkEvent, int32 numOfBytes)
{
	ASSERT_CRASH(networkEvent->eventType == EventType::Accept)
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(networkEvent);
	ProcessAccept(acceptEvent);
}

bool Listener::Start()
{
	if (Listen() == false)
		return false;

	return true;
}

bool Listener::Listen()
{
    // Socket
    _listenSocket = SocketUtil::CreateSocket();
    if (_listenSocket == INVALID_SOCKET)
        return false;

    // Set SocketOpt: 주소 재사용 가능(개발 편함용)
    if (SocketUtil::SetReuseAddress(_listenSocket, true) == false)
        return false;

    // Set SocketOpt: 잉여 송신 데이터 무시.
    if (SocketUtil::SetLinger(_listenSocket, 1, 0) == false)
        return false;

    // Set SocketOpt: 네이글 알고리즘 비활성화
    if (SocketUtil::SetTcpNoDelay(_listenSocket, false) == false)
        return false;

    // 소켓을 IOCP에 등록
    if (_service->GetIocpCore()->RegisterSocket(_listenSocket) == false)
        return false;

    // Bind 
    if (SocketUtil::Bind(_listenSocket, _service->GetNetAddress()) == false)
        return false;

    // Listen
    if (SocketUtil::Listen(_listenSocket) == false)
        return false;

    cout << "Success to generate listen Socket" << '\n';
    
    // GetMaxSessionCount개의 acceptEx를 미리 걸어둔다.
    const int32 acceptCount = _service->GetMaxSessionCount();
    for (int32 i = 0; i < acceptCount; i++)
    {
        AcceptEvent* acceptEvent = new AcceptEvent();
        acceptEvent->owner = shared_from_this();
        _acceptEvents.push_back(acceptEvent);
        RegisterAccept(acceptEvent);
    }

    cout << "Success to register AcceptEvent: " << acceptCount << '\n';

    return true;
}

void Listener::Close()
{
	if (_closed.exchange(true))
		return;

	// 값을 INVALID_SOCKET으로 되돌리지 않는다. 워커가 RegisterAccept에서 같은 변수를 읽고 있을 수 있다.
	// 닫힌 소켓으로 건 AcceptEx는 실패하고, 그 실패 경로가 _closed를 보고 멈춘다.
	::closesocket(_listenSocket);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
	// 리스너를 닫았으면 접속을 더 받지 않는다. 받던 세션은 놓아 준다.
	if (_closed.load())
	{
		acceptEvent->session = nullptr;
		return;
	}

	SessionRef session = _service->CreateSession();
	_service->GetIocpCore()->RegisterSocket(session->GetSocket());

	acceptEvent->session = session;

	DWORD bytesReceived = 0;
	if (false == SocketUtil::AcceptEx(_listenSocket, session->GetSocket(), session->_recvBuffer.Buffer(), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, OUT &bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 일단 다시 Accept 걸어준다
			RegisterAccept(acceptEvent);
		}
	}
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent)
{
	// 리스너를 닫으면 걸어 둔 AcceptEx가 실패로 완료되어 여기로 온다. 다시 걸지 않는다.
	if (_closed.load())
	{
		acceptEvent->session = nullptr;
		return;
	}

	cout << "New Client Arrived" << '\n';

	SessionRef session = acceptEvent->session;

	if (false == SocketUtil::SetUpdateAcceptSocket(session->GetSocket(), _listenSocket))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress; // 상대 호스트 주소.
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));
	session->ProcessConnect();

    // Accept가 완료되자마자 바로 AcceptEx를 다시 걸어준다. 
    // GetMaxSessionCount가 최대 세션 개수를 의미하지 않는다는 것이다.
	RegisterAccept(acceptEvent);
}