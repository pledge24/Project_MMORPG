#pragma once

#include <string_view>

namespace JsonProperty
{
    namespace Item
    {
        constexpr std::string_view TemplateId = "templateId";
        constexpr std::string_view ItemName = "itemName";
        constexpr std::string_view ItemType = "itemType";
        constexpr std::string_view ItemSubtype = "itemSubtype";
        constexpr std::string_view LevelRequirement = "levelRequirement";
        constexpr std::string_view ClassRequirement = "classRequirement";
        constexpr std::string_view BuyPrice = "buyPrice";
        constexpr std::string_view SellPrice = "sellPrice";
        constexpr std::string_view Sellable = "sellable";
        constexpr std::string_view Stackable = "stackable";
        constexpr std::string_view MaxStack = "maxStack";
        constexpr std::string_view Cooldown = "cooldown";
        constexpr std::string_view PhysicalAttack = "physicalAttack";
        constexpr std::string_view MagicalAttack = "magicalAttack";
        constexpr std::string_view Hp = "hp";
        constexpr std::string_view Mp = "mp";
        constexpr std::string_view HpRegenerate = "hpRegenerate";
        constexpr std::string_view MpRegenerate = "mpRegenerate";
        constexpr std::string_view HpRestore = "hpRestore";
        constexpr std::string_view MpRestore = "mpRestore";
    }

    namespace Monster
    {
        constexpr std::string_view TemplateId = "templateId";
        constexpr std::string_view Level = "level";
        constexpr std::string_view MonsterName = "monsterName";
        constexpr std::string_view AttackType = "attackType";
        constexpr std::string_view MaxHp = "maxHp";
        constexpr std::string_view BaseAttack = "baseAttack";
        constexpr std::string_view ExpReward = "expReward";
        constexpr std::string_view MinExp = "minExp";
        constexpr std::string_view MaxExp = "maxExp";
        constexpr std::string_view GoldReward = "goldReward";
        constexpr std::string_view AttackSpeed = "attackSpeed";
        constexpr std::string_view AttackRange = "attackRange";
        constexpr std::string_view DetectionRange = "detectionRange";
        constexpr std::string_view ChaseRange = "chaseRange";
    }

    namespace LevelTable
    {
        constexpr std::string_view Level = "level";
        constexpr std::string_view MaxHp = "maxHp";
        constexpr std::string_view MaxMp = "maxMp";
        constexpr std::string_view PhysicalAttack = "physicalAttack";
        constexpr std::string_view MagicalAttack = "magicalAttack";
        constexpr std::string_view ExpRequirement = "expRequirement";
    }

    namespace Map
    {
        constexpr std::string_view TemplateId = "templateId";
        constexpr std::string_view MapName = "mapName";
        constexpr std::string_view MapType = "mapType";
        constexpr std::string_view HaveSpawnPoint = "haveSpawnPoint";
        constexpr std::string_view CenterPos = "centerPos";
        constexpr std::string_view WidthHalfExtent = "widthHalfExtent";
        constexpr std::string_view HeightHalfExtent = "heightHalfExtent";
        constexpr std::string_view SpawnPoint = "spawnPoint";
        constexpr std::string_view PosX = "posX";
        constexpr std::string_view PosY = "posY";
        constexpr std::string_view PosZ = "posZ";
        constexpr std::string_view Portals = "portals";
        constexpr std::string_view Lists = "lists";
        constexpr std::string_view PortalId = "portalId";
        constexpr std::string_view Type = "type";
        constexpr std::string_view Src = "src";
        constexpr std::string_view Dst = "dst";
        constexpr std::string_view MonsterIds = "monsterIds";
        constexpr std::string_view MaxMonsterCount = "maxMonsterCount";
        // constexpr std::string_view MinMonsterCount = "minMonsterCount";
        constexpr std::string_view RespawnTime = "monsterRespawnTime";
        //constexpr std::string_view SummonCount = "summonCount";

    }
}