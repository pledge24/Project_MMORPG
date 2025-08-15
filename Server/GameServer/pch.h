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

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "ServerPacketHandler.h"
#include "Utils.h"
#include "GameSession.h"

#include "Gamedata.h"

/*-------------------
          DB
---------------------*/
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DBQueue.h"
#include "DBManager.h"
#include "RedisManager.h"
#include "DBRequestFunctions.h"

/*-------------------
         JSON
---------------------*/
#include "nlohmann/json.hpp"
using Json = nlohmann::json;


/*-------------------
       SharedPtr
---------------------*/
USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Monster);
USING_SHARED_PTR(Creature);
USING_SHARED_PTR(Object);
USING_SHARED_PTR(Room);

#define SEND_PACKET(pkt)													    \
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(pkt);	\
	session->Send(sendBuffer);												