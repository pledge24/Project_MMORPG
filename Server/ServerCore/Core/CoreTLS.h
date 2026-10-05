#pragma once

/*--------------------
        CoreTLS
---------------------*/
// TLS에서 사용할 변수들.

extern thread_local uint32				LThreadId;
extern thread_local uint64				LEndTickCount;
extern thread_local class JobQueue*		LCurrentJobQueue;