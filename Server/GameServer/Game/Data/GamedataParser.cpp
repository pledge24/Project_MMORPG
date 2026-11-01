#include "Core/pch.h"
#include "Game/Data/GamedataParser.h"
#include "Game/Data/ActionData.h"

namespace
{
    /** 표 하나를 넘어서는 오류. 메시지에 위치가 이미 들어 있다. */
    struct TableError
    {
        string message;
    };

    //~ 표별 변환

    /**
     * DataBuilder가 생성한 표나 설정을 생성된 역직렬화(DesignData::Parse<표>)로 읽는다.
     * 기획 데이터의 규칙은 DataBuilder가 이미 검사했으므로 여기서는 형식만 본다.
     */
    template<typename Table>
    Table ParseGenerated(const Json& document, optional<string> (*parse)(const Json&, Table&))
    {
        Table table;
        if (optional<string> error = parse(document, OUT table))
            throw TableError{ error.value() };

        return table;
    }

    //~ 배치 데이터

    vector3D ReadPosition(const Json& object, string_view prefix)
    {
        using DesignData::Detail::Read;
        return vector3D(Read<float>(object, format("{}X", prefix)), Read<float>(object, format("{}Y", prefix)), Read<float>(object, format("{}Z", prefix)));
    }

    PlaneBounds ReadBounds(const Json& object)
    {
        using DesignData::Detail::Read;
        return PlaneBounds::FromCenter(Read<float>(object, "CenterX"), Read<float>(object, "CenterY"),
            Read<float>(object, "HalfExtentX"), Read<float>(object, "HalfExtentY"));
    }

    AreaPlacement ReadAreaPlacement(const Json& object)
    {
        AreaPlacement area;
        area.id = DesignData::Detail::Read<int32>(object, "Id");
        area.center = ReadPosition(object, "Center");
        area.bounds = ReadBounds(object);
        return area;
    }

    RespawnPointPlacement ReadRespawnPointPlacement(const Json& object)
    {
        RespawnPointPlacement respawn;
        respawn.areaId = DesignData::Detail::Read<int32>(object, "AreaId");
        respawn.pos = ReadPosition(object, "");
        respawn.yaw = DesignData::Detail::Read<float>(object, "Yaw");
        return respawn;
    }

    PortalPlacement ReadPortalPlacement(const Json& object)
    {
        PortalPlacement portal;
        portal.id = DesignData::Detail::Read<int32>(object, "Id");
        portal.src = ReadPosition(object, "Src");
        portal.radius = DesignData::Detail::Read<float>(object, "Radius");
        portal.dest = ReadPosition(object, "Dest");
        portal.destYaw = DesignData::Detail::Read<float>(object, "DestYaw");
        return portal;
    }

    NpcPlacement ReadNpcPlacement(const Json& object)
    {
        NpcPlacement npc;
        npc.npcId = DesignData::Detail::Read<int32>(object, "Id");
        npc.pos = ReadPosition(object, "");
        npc.yaw = DesignData::Detail::Read<float>(object, "Yaw");
        return npc;
    }

    SpawnPointPlacement ReadSpawnPointPlacement(const Json& object)
    {
        SpawnPointPlacement spawnPoint;
        spawnPoint.spawnGroupId = DesignData::Detail::Read<int32>(object, "SpawnGroupId");
        spawnPoint.pos = ReadPosition(object, "");
        spawnPoint.radius = DesignData::Detail::Read<float>(object, "Radius");
        return spawnPoint;
    }

    /** 행을 번호로 넣는다. 다른 맵의 파일에 같은 번호가 있어도 겹친 것이다. */
    template<typename Placement>
    void AddPlacement(map<int32, Placement>& placements, int32 id, Placement placement, string_view kind)
    {
        if (placements.emplace(id, std::move(placement)).second == false)
            throw DesignData::Detail::FieldError{ format("{} {}이 겹친다", kind, id) };
    }

    /** 포털 표에서 그 포털이 속한 지역의 행을 찾는다. 없으면 nullptr. */
    const DesignData::AreaRow* FindPortalOwner(const DesignData::AreaTable& areas, int32 portalId)
    {
        for (const auto& [areaId, area] : areas)
        {
            for (const DesignData::PortalRow& portal : area.Portals)
            {
                if (portal.Id == portalId)
                    return &area;
            }
        }

        return nullptr;
    }

    /** 배치 데이터의 행이 기획 데이터의 그 맵에 있는 번호를 가리키는지 본다. 위치와 겹침은 추출이 검사했다. */
    void CheckPlacementIds(const MapPlacement& mapPlacement, const vector<AreaPlacement>& areas,
        const vector<RespawnPointPlacement>& respawnPoints, const vector<PortalPlacement>& portals,
        const vector<SpawnPointPlacement>& spawnPoints, const vector<NpcPlacement>& npcs, const GamedataTables& tables)
    {
        using DesignData::Detail::FieldError;
        const int32 mapId = mapPlacement.mapId;
        if (tables.maps.contains(mapId) == false)
            throw FieldError{ format("맵 {}이 맵 표에 없다", mapId) };

        for (const AreaPlacement& area : areas)
        {
            auto row = tables.areas.find(area.id);
            if (row == tables.areas.end())
                throw FieldError{ format("지역 {}이 지역 표에 없다", area.id) };
            if (row->second.MapId != mapId)
                throw FieldError{ format("지역 {}은 지역 표에서 맵 {}에 있다", area.id, row->second.MapId) };
        }

        for (const RespawnPointPlacement& respawn : respawnPoints)
        {
            auto row = tables.areas.find(respawn.areaId);
            if (row == tables.areas.end() || row->second.MapId != mapId)
                throw FieldError{ format("리스폰 지점의 지역 {}이 이 맵의 지역이 아니다", respawn.areaId) };
            if (row->second.HasRespawnPoint == false)
                throw FieldError{ format("지역 {}은 지역 표에서 리스폰 지점이 없는 지역이다", respawn.areaId) };
        }

        for (const PortalPlacement& portal : portals)
        {
            const DesignData::AreaRow* owner = FindPortalOwner(tables.areas, portal.id);
            if (owner == nullptr)
                throw FieldError{ format("포털 {}이 포털 표에 없다", portal.id) };
            if (owner->MapId != mapId)
                throw FieldError{ format("포털 {}은 포털 표에서 맵 {}의 지역 {}에 있다", portal.id, owner->MapId, owner->Id) };
        }

        // 추적 한계는 스폰 지점에서 잰다. 반경이 한계보다 좁아야 반경 안에서 대상을 감지한 몬스터가 쫓아갈 거리가 남는다.
        for (const SpawnPointPlacement& spawnPoint : spawnPoints)
        {
            auto group = tables.spawnGroups.find(spawnPoint.spawnGroupId);
            if (group == tables.spawnGroups.end())
                throw FieldError{ format("스폰 마커의 스폰 그룹 {}이 스폰 그룹 표에 없다", spawnPoint.spawnGroupId) };

            for (const DesignData::SpawnEntryRow& entry : group->second.Entries)
            {
                auto monster = tables.monsters.find(entry.MonsterId);
                if (monster != tables.monsters.end() && spawnPoint.radius >= monster->second.ChasingMaxRange)
                    throw FieldError{ format("스폰 그룹 {}의 마커 반경 {}이 몬스터 {}의 ChasingMaxRange {}보다 좁아야 한다",
                        spawnPoint.spawnGroupId, spawnPoint.radius, entry.MonsterId, monster->second.ChasingMaxRange) };
            }
        }

        for (const NpcPlacement& npc : npcs)
        {
            if (tables.npcs.contains(npc.npcId) == false)
                throw FieldError{ format("NPC {}이 NPC 표에 없다", npc.npcId) };
        }
    }

    /** 맵 하나의 배치 데이터를 읽고 번호를 대조해 표에 더한다. 맵 표와 지역 표를 먼저 읽어 둬야 한다. */
    void ParsePlacement(const string& fileName, const Json& document, GamedataTables& tables)
    {
        using DesignData::Detail::Read;
        using DesignData::Detail::ReadRows;

        try
        {
            if (document.is_object() == false)
                throw DesignData::Detail::FieldError{ "객체가 아니다" };

            MapPlacement mapPlacement;
            mapPlacement.mapId = Read<int32>(document, "MapId");
            mapPlacement.bounds = ReadBounds(document);
            mapPlacement.cellSize = Read<float>(document, "CellSize");
            vector<AreaPlacement> areas = ReadRows<AreaPlacement>(document, "Areas", ReadAreaPlacement);
            vector<RespawnPointPlacement> respawnPoints = ReadRows<RespawnPointPlacement>(document, "RespawnPoints", ReadRespawnPointPlacement);
            vector<PortalPlacement> portals = ReadRows<PortalPlacement>(document, "Portals", ReadPortalPlacement);
            vector<SpawnPointPlacement> spawnPoints = ReadRows<SpawnPointPlacement>(document, "SpawnPoints", ReadSpawnPointPlacement);
            vector<NpcPlacement> npcs = ReadRows<NpcPlacement>(document, "Npcs", ReadNpcPlacement);

            CheckPlacementIds(mapPlacement, areas, respawnPoints, portals, spawnPoints, npcs, tables);

            for (AreaPlacement& area : areas)
                AddPlacement(tables.areaPlacements, area.id, std::move(area), "지역");
            for (RespawnPointPlacement& respawn : respawnPoints)
                AddPlacement(tables.respawnPoints, respawn.areaId, std::move(respawn), "리스폰 지점의 지역");
            for (PortalPlacement& portal : portals)
                AddPlacement(tables.portalPlacements, portal.id, std::move(portal), "포털");
            for (NpcPlacement& npc : npcs)
            {
                npc.mapId = mapPlacement.mapId;
                AddPlacement(tables.npcPlacements, npc.npcId, std::move(npc), "NPC");
            }
            if (spawnPoints.empty() == false)
                tables.spawnPoints[mapPlacement.mapId] = std::move(spawnPoints);
            AddPlacement(tables.mapPlacements, mapPlacement.mapId, std::move(mapPlacement), "맵");
        }
        catch (const DesignData::Detail::FieldError& error)
        {
            throw TableError{ format("{}: {}", fileName, error.cause) };
        }
    }

    /**
     * 기획 데이터의 맵, 지역, 포털, NPC가 모두 배치되어 있는지 본다. 마을에는 리스폰 지점이 있어야 한다.
     * 하나라도 빠지면 그 룸이나 포털, 리스폰, 상점이 서버에서 깨지므로 부팅을 멈춘다.
     */
    void CheckEveryRowPlaced(const GamedataTables& tables)
    {
        for (const auto& [mapId, map] : tables.maps)
        {
            if (tables.mapPlacements.contains(mapId) == false)
                throw TableError{ format("맵 {}의 배치 데이터가 없다. 레벨에서 추출을 돌린다", mapId) };
        }

        for (const auto& [areaId, area] : tables.areas)
        {
            if (tables.areaPlacements.contains(areaId) == false)
                throw TableError{ format("지역 {}의 배치 데이터가 없다. 맵 {}의 레벨에 지역 볼륨을 둔다", areaId, area.MapId) };
            if (area.HasRespawnPoint && tables.respawnPoints.contains(areaId) == false)
                throw TableError{ format("지역 {}에 리스폰 지점의 배치 데이터가 없다", areaId) };

            for (const DesignData::PortalRow& portal : area.Portals)
            {
                if (tables.portalPlacements.contains(portal.Id) == false)
                    throw TableError{ format("포털 {}의 배치 데이터가 없다. 맵 {}의 레벨에 포털을 둔다", portal.Id, area.MapId) };
            }
        }

        for (const auto& [npcId, npc] : tables.npcs)
        {
            if (tables.npcPlacements.contains(npcId) == false)
                throw TableError{ format("NPC {}의 배치 데이터가 없다. 맵 레벨에 NPC 마커를 둔다", npcId) };
        }
    }

    //~ 크리처 판정 데이터

    /** 몬스터나 직업 하나의 행. 번호는 행을 읽는 쪽이 대조한다. */
    pair<int32, CreatureCombat> ReadCreatureCombat(const Json& object)
    {
        using DesignData::Detail::Read;
        CreatureCombat combat;
        combat.capsuleRadius = Read<float>(object, "CapsuleRadius");
        combat.capsuleHalfHeight = Read<float>(object, "CapsuleHalfHeight");
        return { Read<int32>(object, "Id"), combat };
    }

    /** 액션 하나의 행. 번호는 행을 읽는 쪽이 액션 표와 대조한다. */
    pair<int32, ActionTiming> ReadActionTiming(const Json& object)
    {
        using DesignData::Detail::FieldError;
        using DesignData::Detail::Read;

        const int32 actionId = Read<int32>(object, "Id");
        ActionTiming timing;
        timing.montage = Read<string>(object, "Montage");
        timing.section = Read<string>(object, "Section");
        timing.length = Read<float>(object, "Length");
        timing.hitTimes = Read<vector<float>>(object, "HitTimes");

        if (optional<vector<float>> window = Read<optional<vector<float>>>(object, "ComboWindow"))
        {
            if (window->size() != 2)
                throw FieldError{ format("액션 {}: 'ComboWindow'가 시작과 끝 둘이 아니다", actionId) };
            timing.comboWindow = ComboWindow{ (*window)[0], (*window)[1] };
        }

        for (const vector<float>& sample : Read<vector<vector<float>>>(object, "RootMotion"))
        {
            if (sample.size() != 3)
                throw FieldError{ format("액션 {}: 'RootMotion'의 표본이 x, y, z 셋이 아니다", actionId) };
            timing.rootMotion.push_back(vector3D(sample[0], sample[1], sample[2]));
        }
        return { actionId, std::move(timing) };
    }

    /** 직업별 능력치 표에 있는 직업. 이 직업들이 판정 데이터와 짝을 맺는다. */
    set<Protocol::CharacterClass> CollectClasses(const DesignData::ClassLevelStatTable& classLevelStats)
    {
        set<Protocol::CharacterClass> classes;
        for (const auto& [key, row] : classLevelStats)
            classes.insert(get<0>(key));
        return classes;
    }

    /**
     * 크리처 판정 데이터를 읽고 몬스터 표, 직업별 능력치 표, 액션 표와 짝을 대조해 표에 더한다. 세 표를 먼저 읽어 둬야 한다.
     * 짝이 없는 몬스터는 피격을 판정할 캡슐이 없고, 짝이 없는 액션은 언제 타격을 판정할지 모르므로 부팅을 멈춘다.
     * 타이밍의 나머지 검사는 추출이 했으므로, 게임 코드가 타격 순번으로 꺼내 쓰는 타격 시점의 수만 본다.
     */
    void ParseCreatureCombat(const Json& document, GamedataTables& tables)
    {
        using DesignData::Detail::FieldError;
        using DesignData::Detail::Read;
        using DesignData::Detail::ReadRows;

        const set<Protocol::CharacterClass> classes = CollectClasses(tables.classLevelStats);
        try
        {
            if (document.is_object() == false)
                throw FieldError{ "객체가 아니다" };

            tables.rootMotionSampleRate = Read<int32>(document, "RootMotionSampleRate");
            if (tables.rootMotionSampleRate <= 0)
                throw FieldError{ format("'RootMotionSampleRate'가 0보다 커야 하는데 {}이다", tables.rootMotionSampleRate) };

            for (auto& [monsterId, combat] : ReadRows<pair<int32, CreatureCombat>>(document, "Monsters", ReadCreatureCombat))
            {
                const string who = format("몬스터 {}", monsterId);
                if (tables.monsters.contains(monsterId) == false)
                    throw FieldError{ format("{}이 몬스터 표에 없다", who) };
                if (tables.monsterCombats.emplace(monsterId, combat).second == false)
                    throw FieldError{ format("{}이 겹친다", who) };
            }

            for (auto& [classId, combat] : ReadRows<pair<int32, CreatureCombat>>(document, "Classes", ReadCreatureCombat))
            {
                const string who = format("직업 {}", classId);
                const auto characterClass = static_cast<Protocol::CharacterClass>(classId);
                if (Protocol::CharacterClass_IsValid(classId) == false || classes.contains(characterClass) == false)
                    throw FieldError{ format("{}이 직업별 능력치 표에 없다", who) };
                if (tables.classCombats.emplace(characterClass, combat).second == false)
                    throw FieldError{ format("{}이 겹친다", who) };
            }

            for (auto& [actionId, timing] : ReadRows<pair<int32, ActionTiming>>(document, "Actions", ReadActionTiming))
            {
                auto actionIt = tables.actions.find(actionId);
                if (actionIt == tables.actions.end())
                    throw FieldError{ format("액션 {}이 액션 표에 없다", actionId) };

                // 타격 순번 i의 공격 범위를 i번째 타격 시점에 판정한다.
                const size_t hitCount = actionIt->second.Hits.size();
                if (timing.hitTimes.size() != hitCount)
                    throw FieldError{ format("액션 {}: 타격 시점이 {}개인데 액션 표의 타격은 {}개다", actionId, timing.hitTimes.size(), hitCount) };
                if (tables.actionTimings.emplace(actionId, std::move(timing)).second == false)
                    throw FieldError{ format("액션 {}이 겹친다", actionId) };
            }
        }
        catch (const FieldError& error)
        {
            throw TableError{ format("{}: {}", CREATURE_COMBAT_FILE, error.cause) };
        }

        for (const auto& [monsterId, monster] : tables.monsters)
        {
            if (tables.monsterCombats.contains(monsterId) == false)
                throw TableError{ format("몬스터 {}의 크리처 판정 데이터가 없다. 몬스터 에셋 테이블에 행을 두고 추출을 돌린다", monsterId) };
        }
        for (Protocol::CharacterClass characterClass : classes)
        {
            if (tables.classCombats.contains(characterClass) == false)
                throw TableError{ format("직업 {}의 크리처 판정 데이터가 없다. 프로젝트 설정의 CharacterClassPlayers에 직업을 더하고 추출을 돌린다",
                    static_cast<int32>(characterClass)) };
        }
        for (const auto& [actionId, action] : tables.actions)
        {
            if (tables.actionTimings.contains(actionId) == false)
                throw TableError{ format("액션 {}의 크리처 판정 데이터가 없다. 액션 에셋 표에 행을 두고 추출을 돌린다", actionId) };
        }
    }

    //~ 표 사이의 검사

    /**
     * 리스폰 지점이 있는 지역이 마을이다. 마을이 하나뿐인지는 DataBuilder가 검사하므로 여기서는 세지 않는다.
     * 그래도 마을이 없으면 사망한 플레이어와 범위 밖에서 접속한 플레이어를 보낼 곳이 없으므로 부팅을 멈춘다.
     */
    int32 FindTownAreaId(const DesignData::AreaTable& areas)
    {
        for (const auto& [areaId, area] : areas)
        {
            if (area.HasRespawnPoint)
                return areaId;
        }

        throw TableError{ format("{}: 리스폰 지점이 있는 지역(마을)이 없다", DesignData::AREA_FILE) };
    }

    /**
     * 서버는 재화 종류로 골드를 찾는다. GOLD인 행이 하나뿐인지는 DataBuilder가 검사하므로 여기서는 세지 않는다.
     * 그래도 골드가 없으면 DB에 쓸 재화 번호가 없어 서버가 돌 수 없으므로 부팅을 멈춘다.
     */
    int32 FindGoldCurrencyId(const DesignData::CurrencyTable& currencies)
    {
        for (const auto& [currencyId, currency] : currencies)
        {
            if (currency.Type == DesignData::CurrencyType::GOLD)
                return currencyId;
        }

        throw TableError{ format("{}: 재화 종류가 GOLD인 재화가 없다", DesignData::CURRENCY_FILE) };
    }

    /**
     * 몬스터는 대상이 공격 시작 거리 안에 들면 공격을 시작한다. 그 거리가 일반 공격의 공격 범위가 앞으로 닿는 거리를 넘으면
     * 그 사이에 선 대상을 영영 맞히지 못하는 몬스터가 생기므로 부팅을 멈춘다. 일반 공격 액션이 액션 표에 있는지는 DataBuilder가 검사한다.
     */
    void CheckMonsterAttackReach(const DesignData::MonsterTable& monsters, const DesignData::ActionTable& actions)
    {
        for (const auto& [monsterId, monster] : monsters)
        {
            auto actionIt = actions.find(monster.NormalAttackAction);
            if (actionIt == actions.end())
                continue;

            const float reach = ActionData::GetFrontReach(actionIt->second);
            if (monster.TryAttackRange > reach)
                throw TableError{ format("{}: 몬스터 {}의 TryAttackRange {}가 일반 공격 액션 {}의 공격 범위가 앞으로 닿는 거리 {}를 넘는다",
                    DesignData::MONSTER_FILE, monsterId, monster.TryAttackRange, monster.NormalAttackAction, reach) };
        }
    }
}

optional<string> GamedataParser::Parse(const GamedataDocuments& documents, OUT GamedataTables& tables)
{
    try
    {
        tables.currencies = ParseGenerated(documents.currencies, DesignData::ParseCurrency);
        tables.items = ParseGenerated(documents.items, DesignData::ParseItem);
        tables.itemEquipStats = ParseGenerated(documents.itemEquipStats, DesignData::ParseItemEquipStat);
        tables.itemConsumeEffects = ParseGenerated(documents.itemConsumeEffects, DesignData::ParseItemConsumeEffect);
        tables.levels = ParseGenerated(documents.levels, DesignData::ParseLevel);
        tables.classLevelStats = ParseGenerated(documents.classLevelStats, DesignData::ParseClassLevelStat);
        tables.monsters = ParseGenerated(documents.monsters, DesignData::ParseMonster);
        tables.actions = ParseGenerated(documents.actions, DesignData::ParseAction);
        tables.classes = ParseGenerated(documents.classes, DesignData::ParseClass);
        tables.dropGroups = ParseGenerated(documents.dropGroups, DesignData::ParseDropGroup);
        tables.spawnGroups = ParseGenerated(documents.spawnGroups, DesignData::ParseSpawnGroup);
        tables.maps = ParseGenerated(documents.maps, DesignData::ParseMap);
        tables.areas = ParseGenerated(documents.areas, DesignData::ParseArea);
        tables.initialProgress = ParseGenerated(documents.initialProgress, DesignData::ParseProgress);
        tables.shops = ParseGenerated(documents.shops, DesignData::ParseShop);
        tables.npcs = ParseGenerated(documents.npcs, DesignData::ParseNpc);
        tables.interaction = ParseGenerated(documents.interaction, DesignData::ParseInteraction);
        tables.townAreaId = FindTownAreaId(tables.areas);
        tables.goldCurrencyId = FindGoldCurrencyId(tables.currencies);
        CheckMonsterAttackReach(tables.monsters, tables.actions);

        for (const auto& [fileName, document] : documents.placements)
            ParsePlacement(fileName, document, tables);
        CheckEveryRowPlaced(tables);
        ParseCreatureCombat(documents.creatureCombat, tables);
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
