#pragma once

/** 캐릭터 생성 쿼리가 거절한 사유. 클라이언트에 보낸다. 값은 쿼리에 바인딩했다가 부호를 뒤집어 돌려받는다. */
enum class CharacterRejection : int32
{
    DUPLICATE_NAME = 1,
    NO_EMPTY_SLOT = 2,
};

/** 생성 화면에 보여 줄 문구. */
inline string_view ToMessage(CharacterRejection rejection)
{
    switch (rejection)
    {
    case CharacterRejection::DUPLICATE_NAME: return "이미 존재하는 캐릭터입니다.";
    case CharacterRejection::NO_EMPTY_SLOT: return "빈 캐릭터 슬롯이 없습니다.";
    }

    return "";
}

/** 캐릭터 생성의 결과. 쿼리가 거절하면 rejection에 사유가 있고 characterId는 0이다. */
struct CreateCharacterResult
{
    int64 characterId = 0;
    optional<CharacterRejection> rejection;
};

/**
 * 계정의 캐릭터 목록을 불러오고, 캐릭터를 만들고 지운다. 세션과 패킷을 모른다. 응답은 호출한 잡이 만든다.
 * 모든 함수가 DB 쿼리를 블로킹으로 실행하므로 DB 큐의 잡 안에서만 부른다.
 * DB 작업이 실패하면 DBError를 던진다. 거절은 예외가 아니라 결과로 돌려준다.
 */
class CharacterListDAO
{
public:
    static void LoadCharacterList(DBConnection& conn, int64 userId, OUT vector<Protocol::CharacterOverview>& characters);
    /** 빈 슬롯이 없거나 이름이 겹치면 쿼리가 거절하고, 그 사유를 결과에 담는다. */
    static CreateCharacterResult CreateCharacter(DBConnection& conn, const Protocol::CharacterOverview& character, int64 userId);
    /** userId와 함께 대조하므로 다른 계정의 캐릭터는 지우지 않는다. 지운 행이 없으면 false다. */
    static bool DeleteCharacter(DBConnection& conn, int64 userId, int64 characterId);
};
