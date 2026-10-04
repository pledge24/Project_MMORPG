#pragma once

/*----------------------
     GameServerGlobal
-----------------------*/

// 외부 소스코드에서의 참조용도
extern const map<string, int32> GClassMappings;
extern atomic<int64> GNextItemUID;

extern class RoomManager* GRoomManager;

// 세션과 계정에 걸린 시간 제한을 돌리는 큐다. 룸 소유 상태와 무관한 타이머를 룸 큐에 섞지 않으려고 따로 둔다.
extern JobQueueRef GSessionJobQueue;