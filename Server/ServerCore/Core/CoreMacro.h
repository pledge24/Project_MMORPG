#pragma once

/*-------------------
 *     CoreMacro
 *------------------*/
/** 매크로 함수 정의 파일. */

//~ Util
#define OUT

//~ Lock
#define MAKE_MANY_LOCKS(count)	mutex _locks[count];
#define MAKE_LOCK				MAKE_MANY_LOCKS(1)
#define	USE_LOCK_IDX(idx)		lock_guard<mutex> lockGuard_##idx(_locks[idx]);
#define USE_LOCK				USE_LOCK_IDX(0)

//~ Debug
#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}

#define CHECK(expr)                 \
{                                   \
    if(!(expr))                     \
    {                               \
        return false;               \
    }                               \
}

//~ DataSize
#define size16(val)		static_cast<int16>(sizeof(val))
#define size32(val)		static_cast<int32>(sizeof(val))
#define len16(arr)		static_cast<int16>(sizeof(arr)/sizeof(arr[0]))
#define len32(arr)		static_cast<int32>(sizeof(arr)/sizeof(arr[0]))