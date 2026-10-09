#pragma once

/** 캐릭터 생성의 결과. 쿼리가 거절하면 rejection에 사유가 있고 characterId는 0이다. */
struct CreateCharacterResult
{
    int64 characterId = 0;
    /** 클라이언트에 보여 줄 거절 사유. 빈 슬롯이 없거나 이름이 겹칠 때 있다. */
    optional<string> rejection;
};

/**
 * 계정의 캐릭터 목록을 불러오고, 캐릭터를 만들고 지운다. 세션과 패킷을 모른다. 응답은 호출한 잡이 만든다.
 * 모든 함수가 DB 쿼리를 블로킹으로 실행하므로 DB 큐의 잡 안에서만 부른다.
 * DB 작업이 실패하면 사유를 로그에 남기고 false를 돌려준다.
 */
class CharacterListDAO
{
public:
    static bool LoadCharacterList(DBConnection& conn, int64 userId, OUT vector<Protocol::CharacterOverview>& characters);
    /** 빈 슬롯이 없거나 이름이 겹치면 쿼리가 거절하고, 그 사유를 result에 담는다. 거절은 실패가 아니므로 true다. */
    static bool CreateCharacter(DBConnection& conn, const Protocol::CharacterOverview& character, int64 userId, OUT CreateCharacterResult& result);
    /** userId와 함께 대조하므로 다른 계정의 캐릭터는 지우지 않는다. 지운 행이 없으면 false다. */
    static bool DeleteCharacter(DBConnection& conn, int64 userId, int64 characterId);
};
