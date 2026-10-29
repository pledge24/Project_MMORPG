#pragma once
#include "Game/Data/Templates.h"

struct GamedataDocuments;

/**
 * 기획 데이터 표의 보관소.
 *
 * 검증을 마친 기획 데이터를 설치하고, 게임 코드가 템플릿 번호로 조회하는 창구가 된다.
 * 설치한 뒤로는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
 */
class Gamedata
{
    //~ 설치
public:
    /** 기획표 파일을 읽어 검증하고 설치한다. 실패하면 사유를 로그에 남기고 false. */
    static bool LoadAllGamedata();

    /** 문서를 검증하고 통과하면 설치한다. 실패하면 사유를 돌려주고 이전 표를 그대로 둔다. */
    static optional<string> Load(const GamedataDocuments& documents);

    /**
     * 표 전체를 바꿔 끼운다. 부팅과 테스트에서만 부른다.
     * 다른 스레드가 Find*로 읽는 중에 부르면 안 된다.
     */
    static void Install(GamedataTables tables);

    //~ 조회
public:
    /** Find*는 없는 번호면 nullptr. */
    static const DesignData::ItemRow* FindItem(int32 templateId);
    static const DesignData::ItemEquipStatRow* FindItemEquipStat(int32 templateId);
    static const DesignData::ItemConsumeEffectRow* FindItemConsumeEffect(int32 templateId);
    static const DesignData::MonsterRow* FindMonster(int32 templateId);
    static const DesignData::DropGroupRow* FindDropGroup(int32 dropGroupId);
    static const DesignData::AreaRow* FindArea(int32 areaId);
    /** 그 직업의 그 레벨 능력치. 표에 없는 직업이나 레벨이면 nullptr. */
    static const DesignData::ClassLevelStatRow* FindClassLevelStat(int32 classId, int32 level);
    /** 직업별 능력치 표에 그 직업이 있으면 true. 있으면 모든 레벨에 행이 있다. */
    static bool HasClassLevelStats(int32 classId) { return FindClassLevelStat(classId, 1) != nullptr; }

    /** 레벨마다 다음 레벨까지 필요한 경험치. 직업과 무관하다. */
    static const DesignData::LevelTable& GetLevels() { return s_tables.levels; }
    /** 레벨 표의 마지막 레벨. 표가 비어 있으면 0. */
    static int32 GetMaxLevel() { return s_tables.levels.empty() ? 0 : s_tables.levels.rbegin()->first; }

    /** 지역 번호 순서다. */
    static const DesignData::AreaTable& GetAreas() { return s_tables.areas; }
    /** 사망한 플레이어가 돌아가는 마을의 지역 번호. */
    static int32 GetTownAreaId() { return s_tables.townAreaId; }

    /** 배치 데이터. Find*는 없는 번호면 nullptr. 기동할 때 기획 데이터의 맵, 지역, 포털마다 있는지 대조한다. */
    static const AreaPlacement* FindAreaPlacement(int32 areaId);
    static const PortalPlacement* FindPortalPlacement(int32 portalId);
    /** 그 지역의 리스폰 지점. 리스폰 지점이 없는 지역이면 nullptr. */
    static const RespawnPointPlacement* FindRespawnPoint(int32 areaId);

    /** 크리처 판정 데이터. 없는 번호면 nullptr. 기동할 때 몬스터 표의 몬스터와 직업별 능력치 표의 직업마다 있는지 대조한다. */
    static const CreatureCombat* FindMonsterCombat(int32 templateId);
    static const CreatureCombat* FindClassCombat(Protocol::CharacterClass classId);

    /** 골드인 재화의 번호. DB의 CurrencyId로 쓴다. */
    static int32 GetGoldCurrencyId() { return s_tables.goldCurrencyId; }
    /** 새 캐릭터가 첫 게임 입장에서 갖는 시작 맵, 위치, 방향, 골드. */
    static const DesignData::ProgressConfig& GetInitialProgress() { return s_tables.initialProgress; }

    //~ 공통
private:
    static GamedataTables s_tables;
};
