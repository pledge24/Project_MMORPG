#include "ServerCore/Core/pch.h"
#include "ServerCore/Network/IocpCore.h"
#include "ServerCore/Network/NetworkEvent.h"

/*-----------------
	  IocpCore
------------------*/

IocpCore::IocpCore()
{
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	ASSERT_CRASH(_iocpHandle != INVALID_HANDLE_VALUE)
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
		iocpObject->Dispatch(networkEvent, static_cast<int32>(numOfBytes));
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
			cout << "errcode: " << errCode << '\n'; // 64
		    
		    // 실패한 완료도 owner의 Dispatch로 넘겨 onwer가 이를 처리하도록 한다.
		    // owner는 numOfBytes 0으로 실패를 판단할 수 있다.
			IocpObjectRef iocpObject = networkEvent->owner;
			iocpObject->Dispatch(networkEvent, static_cast<int32>(numOfBytes));
			break;
		}
	}

	return true;
}
