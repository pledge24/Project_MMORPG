#pragma once
#include "ThreadManager.h"

/*----------------------
		CoreGlobal
-----------------------*/
/** 전역 객체들을 모아둔 파일. 전역 객체들은 유일하다(unique). */

//~ Thread-Job 관련
extern class ThreadManager* GThreadManager;
extern class GlobalQueue* GGlobalQueue;
extern class JobTimer* GJobTimer;

//~ 저장소 관련
extern class DBConnectionPool* GDBConnectionPool;
extern class DBManager* GDBManager;
extern class RedisManager* GRedisManager;
