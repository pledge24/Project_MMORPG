#include "ServerCore/Core/pch.h"
#include "ServerCore/Core/CoreGlobal.h"
#include "ServerCore/Network/SocketUtil.h"
#include "ServerCore/Job/GlobalQueue.h"
#include "ServerCore/Job/JobTimer.h"
#include "ServerCore/DB/DBConnectionPool.h"
#include "ServerCore/DB/DBManager.h"
#include "ServerCore/DB/RedisManager.h"
#include "ServerCore/Utils/Logger.h"

// 전역 객체 추가 시, 여기에 하나씩 기입
ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

DBConnectionPool* GDBConnectionPool = nullptr;
DBManager* GDBManager = nullptr;
RedisManager* GRedisManager = nullptr;

Logger* GLogger = nullptr;

// 전역 객체를 만들고 지우는 정적 객체. GCoreGlobal 하나가 main보다 먼저 만들어지고 프로세스가 끝날 때 소멸한다.
// 전역 객체를 추가하면 생성자와 소멸자에 함께 넣는다.
// GLogger는 다른 전역 객체가 만들어지고 지워지는 동안에도 쓸 수 있도록 가장 먼저 만들고 가장 나중에 지운다.
class CoreGlobal
{
public:
	CoreGlobal()
	{
		GLogger = new Logger(cout);
		GThreadManager = new ThreadManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
        GDBConnectionPool = new DBConnectionPool();
        GDBManager = new DBManager();
        GRedisManager = new RedisManager();
		SocketUtil::Init();
        wcout.imbue(locale("kor"));
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
        delete GDBConnectionPool;
        delete GDBManager;
        delete GRedisManager;
		SocketUtil::Clear();
		delete GLogger;
	}
} GCoreGlobal;