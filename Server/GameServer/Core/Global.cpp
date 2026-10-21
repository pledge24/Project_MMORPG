#include "Core/pch.h"
#include "Core/Global.h"

// 포인터 전역을 추가하면 ServerContext의 생성자와 소멸자에도 넣는다. 빠뜨리면 아무도 채우지 않아 nullptr로 남는다.
// 설정이 있어야 만들 수 있는 전역(GIdGenerator)은 생성자 대신 ServerContext의 Create* 함수가 채운다.
RoomManager* GRoomManager = nullptr;
SnowflakeIdGenerator* GIdGenerator = nullptr;
JobQueueRef GSessionJobQueue = nullptr;
GameSessionManager* GSessionManager = nullptr;
ProgressCoordinator* GProgressCoordinator = nullptr;
