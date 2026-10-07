#include "ServerCore/Core/pch.h"
#include "ServerCore/Network/Session.h"
#include "ServerCore/Network/SocketUtil.h"
#include "ServerCore/Network/NetworkEvent.h"
#include "ServerCore/Network/Service.h"

/*---------------------
		Session
----------------------*/

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtil::CreateSocket();
}

Session::~Session()
{
	SocketUtil::Close(_socket);
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(NetworkEvent* networkEvent, int32 numOfBytes)
{
	switch (networkEvent->eventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::Disconnect:
		ProcessDisconnect();
		break;
	case EventType::Recv:
		ProcessRecv(numOfBytes);
		break;
	case EventType::Send:
		ProcessSend(numOfBytes);
		break;
	default:
		// TODO
		break;
	}
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const char* cause)
{
    // 설계 특성상 여러 스레드가 동시에 진입할 수 있다. atomic으로 한 번만 실행하도록 막아준다.
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	cout << "Disconnect : " << cause << endl;

	// TODO: 강제 연결 해제 Log 남기기.

	RegisterDisconnect();
}

void Session::DisconnectAfterSend(const char* cause)
{
	{
		USE_LOCK

		if (_disconnectAfterSendCause != nullptr)
			return;

		// 송신이 걸려 있으면 ProcessSend가 큐를 비운 뒤 끊는다. Disconnect를 바로 부르면 _connected가 내려가
		// 큐에서 기다리던 패킷을 RegisterSend가 버린다.
		if (_sendRegistered.load())
		{
			_disconnectAfterSendCause = cause;
			return;
		}
	}

	Disconnect(cause);
}

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어준다.
	{
		USE_LOCK

		// 연결을 끊기로 한 세션이므로 송신하지 않는다.
		if (_disconnectAfterSendCause != nullptr)
			return;

		_sendQueue.push(sendBuffer);
	    
		if (_sendRegistered.exchange(true) == false)
			registerSend = true;

        if (registerSend)
            RegisterSend();
	}
}


bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;

	if (SocketUtil::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtil::BindAnyAddress(_socket, 0/*남는거*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.owner = shared_from_this(); // ADD_REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = GetService()->GetNetAddress().GetSockAddr();
	if (false == SocketUtil::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.owner = shared_from_this(); // ADD_REF

	if (false == SocketUtil::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.owner = nullptr; // RELEASE_REF
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	_recvEvent.Init();
	_recvEvent.owner = shared_from_this(); // ADD_REF

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;

	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.owner = nullptr; // RELEASE_REF
		}
	}
}

void Session::RegisterSend()
{
	if (IsConnected() == false)
		return;

	_sendEvent.Init();
	_sendEvent.owner = shared_from_this(); // ADD_REF

	// 보낼 데이터를 sendEvent에 등록
	{
		// USE_LOCK; <- RegisterSend를 호출하는 함수에서 걸고 진입하기 때문에 안 씀.

		int32 writeSize = 0;
		while (_sendQueue.empty() == false)
		{
			SendBufferRef sendBuffer = _sendQueue.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : 예외 체크

			_sendQueue.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// Scatter-Gather (흩어져 있는 데이터들을 모아서 한 방에 보낸다)
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferRef sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_sendEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.owner = nullptr; // RELEASE_REF
			_sendEvent.sendBuffers.clear(); // RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

// 양측은 연결 성공 시, Recv 이벤트를 등록한다.
void Session::ProcessConnect()
{
	_connectEvent.owner = nullptr; // RELEASE_REF

	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv();
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.owner = nullptr; // RELEASE_REF

	OnDisconnected();
	GetService()->RemoveSession(GetSessionRef());
}

void Session::ProcessRecv(int32 numOfBytes)
{
	_recvEvent.owner = nullptr; // RELEASE_REF

    // 수신한 바이트가 0이라는건 오류가 발생했음을 의미한다. 연결을 끊어준다.
	if (numOfBytes == 0)
	{
		Disconnect("Recv 0");
		return;
	}

	if (_recvBuffer.OnWrite(numOfBytes) == false)
	{
		Disconnect("OnWrite Overflow");
		return;
	}

	int32 unreadSize = _recvBuffer.UnreadSize();

	// processLen: 잘라가서 처리한 패킷 크기.
	int32 processLen = OnRecv(_recvBuffer.ReadPos(), unreadSize);
	if (processLen < 0 || unreadSize < processLen || _recvBuffer.OnRead(processLen) == false)
	{
		Disconnect("OnRead Overflow");
		return;
	}

	// 커서 정리
	_recvBuffer.Clean();

	// 수신 등록
	RegisterRecv();

}

void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.owner = nullptr; // RELEASE_REF
	_sendEvent.sendBuffers.clear(); // RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect("Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	const char* disconnectCause = nullptr;
	{
		USE_LOCK;

		if (_sendQueue.empty())
		{
			_sendRegistered.store(false);
			disconnectCause = _disconnectAfterSendCause;
		}
		else
		{
			RegisterSend();
		}
	}

	// 끊기로 한 세션의 마지막 송신이 끝났다. Disconnect는 락 밖에서 부른다.
	if (disconnectCause != nullptr)
		Disconnect(disconnectCause);
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:		/* 10054: 연결이 원격 호스트에 의해 강제로 끊김*/
	case WSAECONNABORTED:	/* 10053: 소프트웨어에 의한 연결 중단(ex. TCP timeout)*/
		Disconnect("HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

/*---------------------
	  PacketSession
----------------------*/

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;

		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));

		// size는 헤더를 포함한 길이다. 헤더보다 작으면 processLen이 늘지 않거나 덜 늘어 스트림이 깨진다.
		// 음수를 돌려주면 ProcessRecv가 연결을 끊는다.
		if (header.size < sizeof(PacketHeader))
			return -1;

		if (dataSize < header.size)
			break;

		// ---온전한 패킷 하나 조립 성공 시 진입---

		OnRecvPacket(&buffer[processLen], header.size); /* 컨텐츠 코드에서 재정의 */

		processLen += header.size;
	}

	return processLen;
}
