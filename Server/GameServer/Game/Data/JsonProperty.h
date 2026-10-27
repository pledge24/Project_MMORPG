#pragma once

#include <string_view>

namespace JsonProperty
{
    /** 좌표 객체의 필드. */
    namespace Pos
    {
        constexpr std::string_view PosX = "posX";
        constexpr std::string_view PosY = "posY";
        constexpr std::string_view PosZ = "posZ";
    }

    namespace InitialProgress
    {
        constexpr std::string_view StartMapId = "startMapId";
        constexpr std::string_view StartPos = "startPos";
        constexpr std::string_view StartYaw = "startYaw";
        constexpr std::string_view StartGold = "startGold";
    }

    namespace InventorySection
    {
        constexpr std::string_view ItemType = "itemType";
        constexpr std::string_view SlotCount = "slotCount";
    }

    namespace QuickSlot
    {
        constexpr std::string_view SlotCount = "slotCount";
    }
}