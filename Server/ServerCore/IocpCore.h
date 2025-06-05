#pragma once

/*----------------
	IocpObject
-----------------*/

/* Listener / Session의 인터페이스 */
class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() = 0;
	virtual void Dispatch(class NetworkEvent* iocpEvent, int32 numOfBytes = 0) = 0;
};

/*-----------------
	  IocpCore
------------------*/
class IocpCore
{
public:
	IocpCore();
	~IocpCore();

public:
	bool		RegisterSocket(SOCKET socket);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocpHandle;
};

