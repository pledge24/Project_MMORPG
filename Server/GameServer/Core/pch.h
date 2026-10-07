#pragma once

/** 거의 사용되지 않는 내용을 Windows 헤더에서 제외한다. */
#define WIN32_LEAN_AND_MEAN             

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

//~ Core
#include "CorePch.h"
#include "Types.h"
#include "Macro.h"
#include "Global.h"

//~ DB
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DBQueue.h"
#include "DBManager.h"
#include "RedisManager.h"

//~ GameData
#include "Gamedata.h"
#include "JsonProperty.h"

//~ Network
#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "ServerPacketHandler.h"
#include "Utils.h"
#include "GameSession.h"
#include "RoomManager.h"
