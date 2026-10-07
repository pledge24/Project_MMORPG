#pragma once

/**
 * 계정의 캐릭터 목록을 불러오고, 캐릭터를 만들고 지운다. 결과 패킷(S_LOGIN, S_CREATE_CHARACTER,
 * S_DELETE_CHARACTER)을 세션에 보낸다. 성공하든 실패하든 응답을 보낸다.
 * 모든 함수가 DB 쿼리를 블로킹으로 실행하므로 DB 큐의 잡 안에서만 부른다.
 */
class CharacterListDAO
{
public:
    static void LoadCharacterList(SessionRef session, int64 userId);
    /** 빈 슬롯이 없거나 이름이 겹치면 쿼리가 거절하고, 그 사유를 응답에 담는다. */
    static void CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId);
    /** 세션의 _userId와 함께 대조하므로 다른 계정의 캐릭터는 지우지 않는다. */
    static void DeleteCharacter(SessionRef session, int64 characterId);
};
