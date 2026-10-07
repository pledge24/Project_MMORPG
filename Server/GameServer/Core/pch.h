#pragma once

#define WIN32_LEAN_AND_MEAN             

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#include "CorePch.h"
#include "Global.h"

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "ServerPacketHandler.h"
#include "Utils.h"
#include "GameSession.h"
#include "RoomManager.h"

//~ GameData
#include "Gamedata.h"
#include "JsonProperty.h"

//~ DB
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DBQueue.h"
#include "DBManager.h"
#include "RedisManager.h"

//~ Vendor Lib
#include "nlohmann/json.hpp"
using namespace google::protobuf;

//~ GameServer
#include "Types.h"
#include "Macro.h"