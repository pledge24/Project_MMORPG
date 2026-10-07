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
#include "ServerCore/Core/CorePch.h"
#include "Core/Types.h"
#include "Core/Macro.h"
#include "Core/Global.h"

//~ DB
#include "ServerCore/DB/DBConnectionPool.h"
#include "ServerCore/DB/DBBind.h"
#include "ServerCore/DB/DBQueue.h"
#include "ServerCore/DB/DBManager.h"
#include "ServerCore/DB/RedisManager.h"

//~ GameData
#include "Game/Data/Gamedata.h"
#include "Game/Data/JsonProperty.h"

//~ Network
#include "Protocol/Protocol.pb.h"
#include "Protocol/Enum.pb.h"
#include "Protocol/Struct.pb.h"
#include "Network/ServerPacketHandler.h"
#include "Utils/Utils.h"
#include "Network/GameSession.h"
#include "Game/Room/RoomManager.h"
