#include "ServerCore/Core/pch.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/Session.h"
#include "ServerCore/Network/Listener.h"

/*-------------------
		Service
--------------------*/

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory), _maxSessionCount(maxSessionCount)
{

}

Service::~Service()
{
}

void Service::CloseService()
{
}

void Service::DisconnectAllSessions(const char* cause)
{
	// 끊기 완료는 다른 워커가 RemoveSession으로 집합을 고친다. 락 밖에서 끊도록 사본을 뜬다.
	vector<SessionRef> sessions;
	{
		USE_LOCK
		sessions.assign(_sessions.begin(), _sessions.end());
	}

	for (const SessionRef& session : sessions)
		session->Disconnect(cause);
}

int32 Service::GetCurrentSessionCount()
{
	USE_LOCK
	return _sessionCount;
}

void Service::Broadcast(SendBufferRef sendBuffer)
{
	USE_LOCK
	for (const auto& session : _sessions)
	{
	    // Send에서도 락을 건다. Broadcast -> Send순으로 락이 걸림에 주의.
		session->Send(sendBuffer);
	}
}

SessionRef Service::CreateSession()
{
	SessionRef session = _sessionFactory();
	session->SetService(shared_from_this());

	if (_iocpCore->RegisterSocket(session->GetSocket()) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionRef session)
{
	USE_LOCK
	_sessionCount++;
	_sessions.insert(session);
}

void Service::RemoveSession(SessionRef session)
{
	USE_LOCK
    // 서비스에 등록되지 않은 세션을 지우는 경우가 발생해선 안된다. (ASSERT_CRASH는 좀 과할 수 있다)
	ASSERT_CRASH(_sessions.erase(session) != 0)
	_sessionCount--;
}

/*-------------------
	ClientService
--------------------*/

ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Client, targetAddress, core, factory, maxSessionCount)
{
}

bool ClientService::Start()
{
	if (CanStart() == false)
		return false;

	const int32 sessionCount = GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; i++)
	{
		SessionRef session = CreateSession();
		if (session->Connect() == false)
			return false;
	}

	return true;
}

/*-------------------
	ServerService
--------------------*/

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount)
	: Service(ServiceType::Server, address, core, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (CanStart() == false)
		return false;

	_listener = make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	_listener->SetService(static_pointer_cast<ServerService>(shared_from_this()));

	if (_listener->Start() == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	if (_listener != nullptr)
		_listener->Close();

	Service::CloseService();
}
