#include "pch.h"
#include "CoreGlobal.h"
#include "SocketUtil.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"

// 전역 객체 추가 시, 여기에 하나씩 기입
ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

DBConnectionPool* GDBConnectionPool;

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
		SocketUtil::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
        delete GDBConnectionPool;
		SocketUtil::Clear();
	}
} GCoreGlobal;