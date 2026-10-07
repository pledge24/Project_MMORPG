#pragma once

using DataTable = unordered_map<int32, Json>;

/**
 * Json 형태로 되어있는 게임 기획 데이터를 가져와 저장하는 클래스.
 * 
 */
class Gamedata
{
public:
    /** 모든 게임 기획 데이터를 가져온다. */
    static bool LoadAllGamedata();
#ifdef _DEBUG
    static void PrintAllGamedata();
#endif

public:
    /* 직업별 레벨 테이블 */
    static DataTable s_invalidLevelDataTable;
    static DataTable s_warriorLevelDataTable;

    /* 레벨 테이블 매핑 */
    static unordered_map<int32, DataTable*> s_classLevelDataTableMappings;

    // 직업의 레벨 표를 찾는다. 없으면 nullptr.
    // 전역 표는 여러 스레드가 함께 읽는다. operator[]는 없는 키를 끼워 넣으므로 이 함수로만 조회한다.
    static const DataTable* FindClassLevelTable(int32 classId);

    // 아이템 표에서 찾는다. 없는 번호면 nullptr. 레벨 표와 같은 이유로 이 함수로만 조회한다.
    static const Json* FindItemData(int32 templateId);

    /* 게임 데이터 */
    static DataTable s_itemDataTable;
    static DataTable s_mapDataTable;
    static DataTable s_monsterDataTable;
    static DataTable s_questDataTable;
};