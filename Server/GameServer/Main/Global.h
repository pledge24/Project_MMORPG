#pragma once

/*----------------------
     GameServerGlobal
-----------------------*/

// 외부 소스코드에서의 참조용도
extern const map<string, int32> GClassMappings;
extern atomic<int64> GNextItemUID;

extern class RoomManager* GRoomManager;