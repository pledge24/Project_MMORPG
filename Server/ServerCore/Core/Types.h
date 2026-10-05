#pragma once

#include <mutex>
#include <atomic>
#include "sw/redis++/redis.h"

/*-------------------
         Type
---------------------*/
// 타입 컨벤션 통일을 위한 파일. 타입 별칭을 정의한다.

//~ Primitive Type(언리얼과 동일한 컨벤션 사용)
using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

//~ Lock
template<typename T>
using Atomic = std::atomic<T>;
using Mutex = std::mutex;
using CondVar = std::condition_variable;
using UniqueLock = std::unique_lock<std::mutex>;
using LockGuard = std::lock_guard<std::mutex>;

//~ SharedPtr
#define USING_SHARED_PTR(name)	using name##Ref = std::shared_ptr<class name>;
USING_SHARED_PTR(ServerService);
USING_SHARED_PTR(ClientService);
USING_SHARED_PTR(IocpCore);
USING_SHARED_PTR(Listener);
USING_SHARED_PTR(IocpObject);
USING_SHARED_PTR(Session);
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(Job);
USING_SHARED_PTR(JobQueue);
USING_SHARED_PTR(DBQueue);
using RedisRef = std::shared_ptr<class sw::redis::Redis>;

