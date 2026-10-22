#pragma once

/** 로비 표(S_Lobby.json)를 검증해 바꾼 값. */
struct LobbyTables
{
    /** 새 계정의 캐릭터 슬롯 수. 1 이상 255 이하다. */
    int32 defaultCharacterSlotCount = 0;
    /** 캐릭터를 만들 수 있는 직업(Protocol::CharacterClass 값). 비어 있지 않고 겹치지 않는다. */
    vector<int32> creatableClasses;
};

/**
 * 로비 기획 데이터의 보관소.
 *
 * 로비만 읽는 기획 데이터(기본 슬롯 수, 생성할 수 있는 직업)를 설치하고 조회하는 창구다. 게임의 기획 데이터와 서로 읽지 않는다.
 * 설치한 뒤로는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
 */
class LobbyData
{
    //~ 설치
public:
    /** 로비 표 파일을 읽어 검증하고 설치한다. 실패하면 사유를 로그에 남기고 false. */
    static bool LoadAll();

    /** 문서를 검증하고 통과하면 설치한다. 실패하면 사유를 돌려주고 이전 표를 그대로 둔다. */
    static optional<string> Load(const Json& document);

    /** 표 전체를 바꿔 끼운다. 부팅과 테스트에서만 부르고, 다른 스레드가 읽는 중에 부르면 안 된다. */
    static void Install(LobbyTables tables);

    //~ 조회
public:
    static int32 GetDefaultCharacterSlotCount() { return s_tables.defaultCharacterSlotCount; }
    static const vector<int32>& GetCreatableClasses() { return s_tables.creatableClasses; }
    static bool IsCreatableClass(int32 classId);

    //~ 공통
private:
    static LobbyTables s_tables;
};
