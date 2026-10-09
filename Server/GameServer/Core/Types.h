#pragma once

/** GameServer 전역 타입 별칭을 정의한다. */

//~ Vendor Libraries
#include "nlohmann/json.hpp"
#include "google/protobuf/repeated_ptr_field.h" 
    
using Json = nlohmann::json;
namespace Protobuf = google::protobuf;

//~ SharedPtr
USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Monster);
USING_SHARED_PTR(Creature);
USING_SHARED_PTR(Entity);
USING_SHARED_PTR(Room);
USING_SHARED_PTR(InventoryComponent);
USING_SHARED_PTR(EquipmentComponent);
USING_SHARED_PTR(TickIntervalTimer);
