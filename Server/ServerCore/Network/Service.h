#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include <functional>

class Listener;

// Service가 접속을 받는 쪽(서버)인지 거는 쪽(클라이언트)인지 나타낸다.
enum class ServiceType : uint8
{
	Server,
	Client
};

// 콘텐츠 쪽 세션 객체(GameSession 등)를 만드는 함수.
using SessionFactory = function<SessionRef(void)>;

/*-------------------
		Service
--------------------*/

// 세션 팩토리와 IocpCore, 연결된 세션 집합을 소유하는 네트워크 진입점.
// 서버는 ServerService, 클라이언트(DummyClient)는 ClientService를 쓴다. shared_from_this를 쓰므로 shared_ptr로 만든다.
// 연결된 세션은 _sessions가 붙잡고, 끊기면 RemoveSession이 놓는다.
class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~Service();

	virtual bool		Start() abstract;
	bool				CanStart() { return _sessionFactory != nullptr; }

	// 아직 구현이 없다.
	virtual void		CloseService();
	void				SetSessionFactory(SessionFactory func) { _sessionFactory = func; }
			
						/* Session 관리 관련*/
	// 서비스 락을 잡은 채 각 세션의 Send를 부른다. 락 순서는 Service → Session이다.
	void				Broadcast(SendBufferRef sendBuffer); // TEMP?
	// 팩토리로 세션을 만들어 IOCP에 등록한다. 등록에 실패하면 nullptr를 돌려준다. 세션 집합에는 넣지 않는다.
	SessionRef			CreateSession();
	// 연결이 완료된 세션을 붙잡는다. Session::ProcessConnect가 부른다.
	void				AddSession(SessionRef session);
	// 끊긴 세션을 놓는다. 마지막 참조였다면 세션이 소멸한다. 집합에 없는 세션이면 크래시한다.
	void				RemoveSession(SessionRef session);
	int32				GetCurrentSessionCount() { return _sessionCount; }
	// 서버에서는 동시에 걸어 두는 AcceptEx의 수이고 접속 수 상한이 아니다. 클라이언트에서는 만들 세션 수다.
	int32				GetMaxSessionCount() { return _maxSessionCount; }

public:
						/* Service 정보 관련 */
	ServiceType			GetServiceType() { return _type; }
	NetAddress			GetNetAddress() { return _netAddress; }
	IocpCoreRef&		GetIocpCore() { return _iocpCore; }

protected:
	MAKE_LOCK;
	ServiceType			_type;
	NetAddress			_netAddress = {};
	IocpCoreRef			_iocpCore;

	set<SessionRef>		_sessions;
	int32				_sessionCount = 0;
	int32				_maxSessionCount = 0;
	SessionFactory		_sessionFactory;
};

/*-------------------
	ClientService
--------------------*/

// 대상 주소로 GetMaxSessionCount()개의 세션을 접속시키는 서비스. DummyClient가 쓴다.
class ClientService : public Service
{
public:
	ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ClientService() {}

	virtual bool	Start() override;
};


/*-------------------
	ServerService
--------------------*/

// Listener를 만들어 접속을 받는 서비스. GameServer가 쓴다.
class ServerService : public Service
{
public:
	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool	Start() override;
	virtual void	CloseService() override;

private:
	ListenerRef		_listener = nullptr;
};