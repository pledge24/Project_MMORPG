#pragma once
#include "Lobby/Data/Generated/DesignData.h"

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
    /** 로비 설정 파일을 읽어 검증하고 설치한다. 실패하면 사유를 로그에 남기고 false. */
    static bool LoadAll();

    /** 문서를 검증하고 통과하면 설치한다. 실패하면 사유를 돌려주고 이전 설정을 그대로 둔다. */
    static optional<string> Load(const Json& document);

    /** 설정 전체를 바꿔 끼운다. 부팅과 테스트에서만 부르고, 다른 스레드가 읽는 중에 부르면 안 된다. */
    static void Install(LobbyDesignData::LobbyConfig config);

    //~ 조회
public:
    static int32 GetDefaultCharacterSlotCount() { return s_config.DefaultCharacterSlotCount; }
    /** 비어 있지 않고 겹치지 않는다(DataBuilder가 검사한다). */
    static const vector<Protocol::CharacterClass>& GetCreatableClasses() { return s_config.CreatableClasses; }
    static bool IsCreatableClass(int32 classId);

    //~ 공통
private:
    static LobbyDesignData::LobbyConfig s_config;
};
