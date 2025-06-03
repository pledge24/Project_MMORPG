#pragma once

/*----------------------
   Thread Local Storage
-----------------------*/

// 외부 소스코드에서 참조용도
extern thread_local uint32				LThreadId;
extern thread_local uint64				LEndTickCount;
extern thread_local class JobQueue*		LCurrentJobQueue;