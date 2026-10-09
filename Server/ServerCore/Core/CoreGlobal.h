#pragma once
#include "ServerCore/Thread/ThreadManager.h"

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

//~ 로그 관련
extern class Logger* GLogger;

/**
 * 위 전역 객체를 만들고 지운다. 프로그램마다 main이 맨 처음에 지역 변수로 하나 만든다.
 * 생성자가 GLogger부터 차례로 만들고, 소멸자가 남은 스레드를 합류시킨 뒤 만든 순서의 역순으로 지운다.
 * 정적 객체로 두지 않는다. 다른 번역 단위의 정적 객체와 만들고 지우는 순서가 정해지지 않는다.
 */
class CoreGlobal
{
public:
    CoreGlobal();
    ~CoreGlobal();

    CoreGlobal(const CoreGlobal&) = delete;
    CoreGlobal& operator=(const CoreGlobal&) = delete;
};
