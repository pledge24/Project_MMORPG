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

void Listener::Dispatch(NetworkEvent* networkEvent, int32 numOfBytes, int32 errorCode)
{
	ASSERT_CRASH(networkEvent->eventType == EventType::Accept)
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(networkEvent);
	ProcessAccept(acceptEvent, errorCode);
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

    // 네이글 알고리즘은 리슨 소켓이 아니라 접속한 세션 소켓에서 끈다(Session::ProcessConnect).

    // 소켓을 IOCP에 등록
    if (_service->GetIocpCore()->RegisterSocket(_listenSocket) == false)
        return false;

    // Bind
    if (SocketUtil::Bind(_listenSocket, _service->GetNetAddress()) == false)
        return false;

    // Listen
    if (SocketUtil::Listen(_listenSocket) == false)
        return false;

    // AcceptEx를 미리 걸어 둔다. 접속 상한(GetMaxSessionCount)이 커도 걸어 두는 수는 MAX_PENDING_ACCEPT_COUNT까지다.
    const int32 acceptCount = min(_service->GetMaxSessionCount(), MAX_PENDING_ACCEPT_COUNT);
    for (int32 i = 0; i < acceptCount; i++)
    {
        AcceptEvent* acceptEvent = new AcceptEvent();
        acceptEvent->owner = shared_from_this();
        _acceptEvents.push_back(acceptEvent);
        RegisterAccept(acceptEvent);
    }

    GLogger->Info("리슨 소켓을 열고 AcceptEx {}개를 걸었다", acceptCount);

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
	// 받던 세션은 놓아 준다. 접속을 받지 못한 세션은 소멸하며 소켓을 닫는다.
	acceptEvent->session = nullptr;

	// 리스너를 닫았으면 접속을 더 받지 않는다.
	if (_closed.load())
		return;

	// CreateSession이 세션 소켓을 IOCP에 등록한다. 여기서 다시 등록하지 않는다.
	SessionRef session = _service->CreateSession();
	if (session == nullptr)
	{
		GLogger->Error("세션 소켓을 IOCP에 등록하지 못해 AcceptEx 하나를 걸지 않는다(오류 {})", ::GetLastError());
		return;
	}

	acceptEvent->session = session;

	DWORD bytesReceived = 0;
	if (false == SocketUtil::AcceptEx(_listenSocket, session->GetSocket(), session->_recvBuffer.Buffer(), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, OUT &bytesReceived, static_cast<LPOVERLAPPED>(acceptEvent)))
	{
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			// 다시 걸지 않는다. 같은 오류가 이어지면 다시 거는 호출이 끝나지 않는다. 걸어 둔 수가 하나 줄어든다.
			acceptEvent->session = nullptr;
			if (_closed.load() == false)
				GLogger->Error("AcceptEx를 걸지 못해 하나를 포기한다(오류 {})", errorCode);
		}
	}
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent, int32 errorCode)
{
	// 리스너를 닫으면 걸어 둔 AcceptEx가 실패로 완료되어 여기로 온다. 다시 걸지 않는다.
	if (_closed.load())
	{
		acceptEvent->session = nullptr;
		return;
	}

	// 받다가 실패한 접속이다. 그 세션은 버리고 새로 건다.
	if (errorCode != 0)
	{
		RegisterAccept(acceptEvent);
		return;
	}

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

    // Accept가 완료되자마자 같은 이벤트로 AcceptEx를 다시 건다. 접속 상한은 Service::AddSession이 본다.
	RegisterAccept(acceptEvent);
}
