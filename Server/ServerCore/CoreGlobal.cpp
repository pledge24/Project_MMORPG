#include "pch.h"
#include "CoreGlobal.h"
#include "SocketUtil.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"
#include "DBManager.h"

// 전역 객체 추가 시, 여기에 하나씩 기입
ThreadManager* GThreadManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

DBConnectionPool* GDBConnectionPool;
DBManager* GDBManager = nullptr;

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
		SocketUtil::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;
        delete GDBConnectionPool;
        delete GDBManager;
		SocketUtil::Clear();
	}
} GCoreGlobal;