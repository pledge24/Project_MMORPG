#pragma once
#include "Game/Data/Templates.h"

struct GamedataDocuments;

/**
 * 기획 데이터 표를 들고 있는 클래스.
 * 부팅할 때 LoadAllGamedata가 한 번 설치하고, 그 뒤로는 바뀌지 않으므로 여러 스레드가 Find*로 함께 읽는다.
 * 표는 private이다. 조회는 Find*로만 한다.
 */
class Gamedata
{
public:
    /** 기획표 파일을 읽어 검증하고 설치한다. 실패하면 사유를 로그에 남기고 false. 이때 서버는 뜨지 않는다. */
    static bool LoadAllGamedata();

    /** 문서를 검증하고 통과하면 설치한다. 실패하면 사유를 돌려주고 이전 표를 그대로 둔다. */
    static optional<string> Load(const GamedataDocuments& documents);

    /**
     * 표 전체를 바꿔 끼운다. 부팅과 테스트에서만 부른다.
     * 다른 스레드가 Find*로 읽는 중에 부르면 안 된다.
     */
    static void Install(GamedataTables tables);

    //~ 조회. 없는 번호면 nullptr.
    static const ItemTemplate* FindItem(int32 templateId);
    static const MonsterTemplate* FindMonster(int32 templateId);
    static const MapTemplate* FindMap(int32 templateId);
    static const ClassLevelTable* FindClassLevelTable(int32 classId);

    /** 룸 번호 순서다. */
    static const map<int32, MapTemplate>& GetMaps() { return s_tables.maps; }
    /** 사망한 플레이어가 돌아가는 마을 룸 번호. */
    static int32 GetTownRoomId() { return s_tables.townRoomId; }

private:
    static GamedataTables s_tables;
};
