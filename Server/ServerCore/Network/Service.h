#pragma once
#include "ServerCore/Network/NetAddress.h"
#include "ServerCore/Network/IocpCore.h"
#include <functional>

class Listener;

/** Service가 접속을 받는 쪽(서버)인지 거는 쪽(클라이언트)인지 나타낸다. */
enum class ServiceType : uint8
{
	Server,
	Client
};

/** 콘텐츠 쪽 세션 객체(GameSession 등)를 만드는 함수. */
using SessionFactory = function<SessionRef(void)>;

/*-------------------
		Service
--------------------*/

/**
 * 네트워크 진입점 역할의 Service 인터페이스. 
 * Iocp 통신에 필요한 요소(세션, IocpCore 등)및 운영 기능이 포함되어 있다.
 * 생성자에서 서비스 필요한 여러 요소를 인자로 건네주는게 특징.
 */
class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	virtual ~Service();

	virtual bool		Start() abstract;
	bool				CanStart() { return _sessionFactory != nullptr; }

	                    /** 새 접속을 더 받지 않는다. 이미 접속한 세션은 그대로 둔다. */
	virtual void		CloseService();
	void				SetSessionFactory(SessionFactory func) { _sessionFactory = func; }
			
	//~ Session 관리 관련
	/** 모든 세션에 패킷을 보낸다. */
	void				Broadcast(SendBufferRef sendBuffer);
	/** 세션 팩토리로 세션을 만들어 IOCP에 등록한다. 등록에 실패하면 nullptr를 돌려준다. */
	SessionRef			CreateSession();
	/** [LOCK] 연결이 완료된 세션을 추가한다. Session::ProcessConnect가 부른다. 접속 상한에 닿았으면 넣지 않고 false. */
	bool				AddSession(SessionRef session);
	/** 연결이 끊긴 세션을 제거한다. */
	void				RemoveSession(SessionRef session);
	/** [LOCK] 지금 연결된 세션 모두에 Disconnect를 건다. 끊기가 끝나면 세션마다 OnDisconnected가 불리고 집합에서 빠진다. */
	void				DisconnectAllSessions(const char* cause);
	/** [LOCK] 연결된 세션 수. 끊기가 끝나 RemoveSession이 불린 세션은 세지 않는다. */
	int32				GetCurrentSessionCount();
	/** 동시에 연결할 수 있는 세션 수. 서버는 넘는 접속을 받지 않고, 클라이언트는 이 수만큼 접속을 건다. */
	int32				GetMaxSessionCount() { return _maxSessionCount; }

public:
	//~ Service 정보 관련
	ServiceType			GetServiceType() { return _type; }
	NetAddress			GetNetAddress() { return _netAddress; }
	IocpCoreRef&		GetIocpCore() { return _iocpCore; }

protected:
	MAKE_LOCK
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

/** 
 * 대상 주소로 GetMaxSessionCount개의 세션을 접속시키는 클라이언트 서비스. 
 * 테스트용이며, DummyClient가 사용한다.
 */
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

/** Listener를 만들어 접속을 받는 서버 서비스. */
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