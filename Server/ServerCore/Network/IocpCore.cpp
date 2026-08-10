#include "pch.h"
#include "IocpCore.h"
#include "NetworkEvent.h"

/*-----------------
	  IocpCore
------------------*/

IocpCore::IocpCore()
{
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE);
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
		iocpObject->Dispatch(networkEvent, numOfBytes);
	}
	else
	{
		int32 errCode = ::WSAGetLastError();
		switch (errCode)
		{
		case WAIT_TIMEOUT:
			return false;
		default:
			// TODO : 로그 찍기
			cout << "errcode: " << errCode << endl; // 64
			IocpObjectRef iocpObject = networkEvent->owner;
			iocpObject->Dispatch(networkEvent, numOfBytes);
			break;
		}
	}

	return true;
}
