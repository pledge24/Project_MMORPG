#include "Core/pch.h"
#include "Core/Global.h"

// 전역 객체 추가 시, 여기에 하나씩 기입. 포인터 전역은 ServerContext의 생성자와 소멸자에도 넣는다.
atomic<int64> GNextItemUID = 0;
RoomManager* GRoomManager = nullptr;
JobQueueRef GSessionJobQueue = nullptr;
GameSessionManager* GSessionManager = nullptr;
ProgressCoordinator* GProgressCoordinator = nullptr;
