#include "ServerCore/Core/pch.h"
#include "ServerCore/Network/IocpCore.h"
#include "ServerCore/Network/NetworkEvent.h"

/*-----------------
	  IocpCore
------------------*/

IocpCore::IocpCore()
{
	// 실패하면 INVALID_HANDLE_VALUE가 아니라 NULL을 돌려준다.
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != NULL)
}

IocpCore::~IocpCore()
{
	::CloseHandle(_iocpHandle);
}

bool IocpCore::RegisterSocket(SOCKET socket)
{
	return CreateIoCompletionPort((HANDLE)socket, _iocpHandle, 0, 0);
}

bool IocpCore::Dispatch(uint32 timeoutMs)
{
	DWORD numOfBytes = 0;
	ULONG_PTR key = 0;
	NetworkEvent* networkEvent = nullptr;

	if (::GetQueuedCompletionStatus(_iocpHandle, OUT &numOfBytes, OUT &key, OUT reinterpret_cast<LPOVERLAPPED*>(&networkEvent), timeoutMs))
	{
		IocpObjectRef iocpObject = networkEvent->owner;
		iocpObject->Dispatch(networkEvent, static_cast<int32>(numOfBytes), 0);
		return true;
	}

	const int32 errorCode = static_cast<int32>(::GetLastError());

	// 완료 패킷을 꺼내지 못했다. 시간이 다 됐거나 포트 자체가 실패했고, 넘길 owner가 없다.
	if (networkEvent == nullptr)
	{
		if (errorCode != WAIT_TIMEOUT)
			GLogger->Error("IOCP에서 완료 패킷을 꺼내지 못했다(오류 {})", errorCode);

		return false;
	}

	// I/O가 실패한 완료다. 처리는 owner가 오류 코드를 보고 정한다.
	// 상대가 끊었거나(64) 소켓을 닫아 취소된(995) I/O는 흔하므로 로그를 남기지 않는다.
	if (errorCode != ERROR_NETNAME_DELETED && errorCode != ERROR_OPERATION_ABORTED)
		GLogger->Warning("I/O가 오류 {}로 끝났다", errorCode);

	IocpObjectRef iocpObject = networkEvent->owner;
	iocpObject->Dispatch(networkEvent, 0, errorCode);
	return true;
}
