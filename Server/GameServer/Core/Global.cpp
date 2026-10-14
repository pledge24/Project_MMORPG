#include "Core/pch.h"
#include "Core/Global.h"

// 포인터 전역을 추가하면 ServerContext의 생성자와 소멸자에도 넣는다. 빠뜨리면 아무도 채우지 않아 nullptr로 남는다.
atomic<int64> GNextItemUID = 0;
RoomManager* GRoomManager = nullptr;
JobQueueRef GSessionJobQueue = nullptr;
GameSessionManager* GSessionManager = nullptr;
ProgressCoordinator* GProgressCoordinator = nullptr;
