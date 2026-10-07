#pragma once

/** GameServer 전역 타입 별칭을 정의한다. pch.h에서 ServerCore와 json 헤더 뒤에 포함한다. */

//~ Vendor Lib
using Json = nlohmann::json;

//~ SharedPtr
USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Monster);
USING_SHARED_PTR(Creature);
USING_SHARED_PTR(Entity);
USING_SHARED_PTR(Room);
USING_SHARED_PTR(Inventory);
USING_SHARED_PTR(EquippedGear);
USING_SHARED_PTR(TickIntervalTimer);
