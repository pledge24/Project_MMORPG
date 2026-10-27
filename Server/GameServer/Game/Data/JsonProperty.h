#pragma once

#include <string_view>

namespace JsonProperty
{
    namespace LevelTable
    {
        constexpr std::string_view Level = "level";
        constexpr std::string_view MaxHp = "maxHp";
        constexpr std::string_view MaxMp = "maxMp";
        constexpr std::string_view PhysicalAttack = "physicalAttack";
        constexpr std::string_view MagicalAttack = "magicalAttack";
        constexpr std::string_view ExpRequirement = "expRequirement";

        constexpr std::string_view MaxHp_Increment = "maxHpIncrement";
        constexpr std::string_view MaxMp_Increment = "maxMpIncrement";
        constexpr std::string_view PA_Increment = "paIncrement";
        constexpr std::string_view MA_Increment = "maIncrement";
    }

    namespace Map
    {
        constexpr std::string_view TemplateId = "templateId";
        constexpr std::string_view MapId = "mapId";
        constexpr std::string_view MapName = "mapName";
        constexpr std::string_view MapType = "mapType";
        constexpr std::string_view HasRespawnPoint = "hasRespawnPoint";
        constexpr std::string_view CenterPos = "centerPos";
        constexpr std::string_view DepthHalfExtent = "depthHalfExtent";
        constexpr std::string_view WidthHalfExtent = "widthHalfExtent";
        constexpr std::string_view RespawnPoint = "respawnPoint";
        constexpr std::string_view PosX = "posX";
        constexpr std::string_view PosY = "posY";
        constexpr std::string_view PosZ = "posZ";
        constexpr std::string_view Portals = "portals";
        constexpr std::string_view Lists = "lists";
        constexpr std::string_view PortalId = "portalId";
        constexpr std::string_view Type = "type";
        constexpr std::string_view Src = "src";
        constexpr std::string_view Dst = "dst";
        constexpr std::string_view PortalRadius = "portalRadius";
        constexpr std::string_view MonsterIds = "monsterIds";
        constexpr std::string_view MaxMonsterCount = "maxMonsterCount";
        constexpr std::string_view Yaw = "yaw";
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