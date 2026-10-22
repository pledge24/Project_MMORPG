#pragma once

/** 캐릭터 생성 쿼리가 거절한 사유. 쿼리가 성공하면 0을 돌려주므로 값은 0이 아니어야 한다. */
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

/** 캐릭터 목록의 한 행. */
struct CharacterListRow
{
    int64 characterId = 0;
    /** 직업. DB의 Character.ClassId 값이다. */
    int32 classId = 0;
    string name;
    /** 마지막으로 저장한 레벨. */
    int32 level = 0;
};

/** 캐릭터 생성의 결과. 쿼리가 거절하면 rejection에 사유가 있고 characterId는 0이다. */
struct CreateCharacterResult
{
    int64 characterId = 0;
    optional<CharacterRejection> rejection;
};

/**
 * 계정의 캐릭터 목록의 DB 접근 창구.
 *
 * 캐릭터 선택 화면에 필요한 목록을 불러오고, 캐릭터를 만들고 지운다.
 * 모든 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다.
 * DB 작업이 실패하면 DBError를 던지고, 요청의 거절은 예외가 아니라 결과로 돌려준다.
 */
class CharacterListDAO
{
public:
    /** 삭제되지 않은 캐릭터를 만든 순서로 돌려준다. */
    static void LoadCharacterList(DBConnection& conn, int64 accountId, OUT vector<CharacterListRow>& characters);
    /**
     * characterId로 캐릭터 행만 만든다. 진행 행은 첫 게임 입장이 만든다. 계정 행이 먼저 있어야 한다(없으면 DBError).
     * 이름과 직업은 CharacterCreation::Validate를 통과한 값만 넘긴다. 빈 슬롯이 없거나 이름이 겹치면 거절 사유를 결과에 담는다.
     */
    static CreateCharacterResult CreateCharacter(DBConnection& conn, int64 accountId, int64 characterId, const string& name, int32 classId);
    /** characterId는 클라이언트 값이어도 된다. 이 계정의 삭제되지 않은 캐릭터가 아니면 지우지 않고 false. */
    static bool DeleteCharacter(DBConnection& conn, int64 accountId, int64 characterId);
    /** characterId는 클라이언트 값이어도 된다. 이 계정의 삭제되지 않은 캐릭터면 true. */
    static bool OwnsCharacter(DBConnection& conn, int64 accountId, int64 characterId);
};
