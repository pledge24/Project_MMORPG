#pragma once
#include "ThreadManager.h"

/*----------------------
		CoreGlobal
-----------------------*/

// 외부 소스코드에서의 참조용도
extern class ThreadManager* GThreadManager;
extern class GlobalQueue* GGlobalQueue;
extern class JobTimer* GJobTimer;

extern class DBConnectionPool* GDBConnectionPool;