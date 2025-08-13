#include "pch.h"
#include "CoreGlobal.h"
#include "SocketUtil.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"
#include "DBManager.h"
#include "RedisManager.h"

// 전역 객체 추가 시, 여기에 하나씩 기입
ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

DBConnectionPool* GDBConnectionPool = nullptr;
DBManager* GDBManager = nullptr;
RedisManager* GRedisManager = nullptr;

/*----------------------
		CoreGlobal
-----------------------*/

class CoreGlobal
{
public:
	CoreGlobal()
	{
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
	}
} GCoreGlobal;