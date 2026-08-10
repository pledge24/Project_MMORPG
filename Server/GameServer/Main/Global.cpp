#include "pch.h"
#include "Global.h"

const map<string, int32> GClassMappings = {
    make_pair("warrior", 1)
};

// 전역 객체 추가 시, 여기에 하나씩 기입
atomic<int64> GNextItemUID = 0;

RoomManager* GRoomManager = nullptr;

/*----------------------
     GameServerGlobal
-----------------------*/

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