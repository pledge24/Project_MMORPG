#include "pch.h"
#include "Listener.h"
#include "NetAddress.h"
#include "Service.h"
#include "SocketUtil.h"
#include "NetworkEvent.h"
#include "Session.h"

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
	ASSERT_CRASH(networkEvent->eventType == EventType::Accept);
	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(networkEvent);
	ProcessAccept(acceptEvent);
}

bool Listener::Start()
{
	if (Listen() == false)
		return false;

	if (Accept() == false)
		return false;

	return true;
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent)
{
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
	cout << "New Client Arrived" << endl;

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

	RegisterAccept(acceptEvent);
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
	if (SocketUtil::SetLinger(_listenSocket, 0, 0) == false)
		return false;

	// Set SocketOpt: 네이글 알고리즘 비활성화
	if (SocketUtil::SetTcpNoDelay(_listenSocket, false) == false)
		return false;

	// IOCP에 등록
	if (_service->GetIocpCore()->RegisterSocket(_listenSocket) == false)
		return false;

	// Bind 
	if (SocketUtil::Bind(_listenSocket, _service->GetNetAddress()) == false)
		return false;

	// Listen
	if (SocketUtil::Listen(_listenSocket) == false)
		return false;

	cout << "Success to generate listen Socket" << endl;

	return true;
}

bool Listener::Accept()
{
	const int32 acceptCount = _service->GetMaxSessionCount();
	for (int32 i = 0; i < acceptCount; i++)
	{
		AcceptEvent* acceptEvent = new AcceptEvent();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	cout << "Success to register AcceptEvent: " << acceptCount << endl;

	return true;
}
