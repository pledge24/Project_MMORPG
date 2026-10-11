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

// 전역 객체를 추가하면 생성자와 소멸자에 함께 넣는다. 소멸자는 생성자의 역순을 지킨다.
// GLogger는 다른 전역 객체가 만들어지고 지워지는 동안에도 쓸 수 있도록 가장 먼저 만들고 가장 나중에 지운다.
CoreGlobal::CoreGlobal()
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

CoreGlobal::~CoreGlobal()
{
    // 아래 객체를 쓰는 스레드가 남아 있으면 지우는 도중에 읽는다. 끝나지 않는 스레드면 여기서 멈춘다.
    GThreadManager->Join();

    SocketUtil::Clear();
    delete GRedisManager;
    GRedisManager = nullptr;
    delete GDBManager;
    GDBManager = nullptr;
    delete GDBConnectionPool;
    GDBConnectionPool = nullptr;
    delete GJobTimer;
    GJobTimer = nullptr;
    delete GGlobalQueue;
    GGlobalQueue = nullptr;
    delete GThreadManager;
    GThreadManager = nullptr;
    delete GLogger;
    GLogger = nullptr;
}