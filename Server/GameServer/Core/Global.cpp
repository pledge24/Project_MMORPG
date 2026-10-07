#include "Core/pch.h"
#include "Core/Global.h"

// 전역 객체 추가 시, 여기에 하나씩 기입
const map<string, int32> GClassMappings = {
    make_pair("warrior", 1)
};

atomic<int64> GNextItemUID = 0;
RoomManager* GRoomManager = nullptr;
JobQueueRef GSessionJobQueue = make_shared<JobQueue>();

class GameServerGlobal
{
public:
    GameServerGlobal()
    {
        GRoomManager = new RoomManager();
    }

    ~GameServerGlobal()
    {
        delete GRoomManager;
    }
} GGameServerGlobal;