#include "Core/pch.h"
#include "Lobby/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Lobby/CharacterCreation.h"

// 이름을 읽는 버퍼는 열 크기로 잡는다. 생성 규칙이 열보다 긴 이름을 받으면 INSERT가 이름을 자르거나 실패한다.
static_assert(CharacterCreation::MAX_NAME_LENGTH == CHARACTER_NAME_COLUMN_LENGTH, "캐릭터 이름 길이 제한과 Character.Name 열 크기가 다르다");

void CharacterListDAO::LoadCharacterList(DBConnection& conn, int64 accountId, OUT vector<CharacterListRow>& characters)
{
    const int PARAMS = 1;
    const int COLS = 4;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 accountId) : _accountId(accountId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _accountId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _characterId);
            dbBind.BindCol(1, _classId);
            dbBind.BindCol(2, _name);
            dbBind.BindCol(3, _level);
        }

        //~ Params
        int64 _accountId;

        //~ Cols
        int64 _characterId = 0;
        int32 _classId = 0;
        WCHAR _name[CHARACTER_NAME_COLUMN_LENGTH + 1] = {};
        int32 _level = 0;
    };


    // 같은 시각에 만든 캐릭터가 있어도 순서가 매번 같도록 CharacterId로 한 번 더 정렬한다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SELECT CharacterId, ClassId, Name, Level
        FROM dbo.[Character]
        WHERE AccountId = (?)
          AND DeletedAt IS NULL
        ORDER BY CreatedAt, CharacterId
    )SQL");

    BindObject bindObject(dbBind, accountId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    while (conn.Fetch())
    {
        CharacterListRow& character = characters.emplace_back();
        character.characterId = bindObject._characterId;
        character.classId = bindObject._classId;
        character.name = EncodingConverter::WCharToString(bindObject._name);
        character.level = bindObject._level;
    }
}

CreateCharacterResult CharacterListDAO::CreateCharacter(DBConnection& conn, int64 accountId, int64 characterId, const string& name, int32 classId)
{
    const int PARAMS = 6;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 accountId, int64 characterId, const string& name, int32 classId)
            : _accountId(accountId), _characterId(characterId), _classId(classId), _name(EncodingConverter::StringToWString(name))
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _accountId);
            dbBind.BindParam(1, _characterId);
            dbBind.BindParam(2, _classId);
            dbBind.BindParam(3, _name.c_str());
            dbBind.BindParam(4, _rejections[0]);
            dbBind.BindParam(5, _rejections[1]);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _result);
        }

        //~ Params
        int64 _accountId;
        int64 _characterId;
        int32 _classId;
        wstring _name;
        // 순서는 쿼리의 @duplicate_name_error, @no_empty_slot_error 순서와 맞아야 한다. 어긋나면 거절 사유가 뒤바뀐다.
        int32 _rejections[2] = { static_cast<int32>(CharacterRejection::DUPLICATE_NAME), static_cast<int32>(CharacterRejection::NO_EMPTY_SLOT) };

        //~ Cols
        int32 _result = -1;
    };


    // 캐릭터 행만 넣는다. 진행 행(상태와 재화)은 첫 게임 입장이 게임의 기획 데이터로 만든다.
    // 계정 행을 잠가 같은 계정의 생성 요청을 한 줄로 세운다. 겹쳐 와도 슬롯 한도를 넘지 않는다.
    // XACT_ABORT가 켜져 있어 어느 문장이 실패해도 트랜잭션 전체가 되돌려진다.
    // 이름 중복은 미리 조회하지 않고 고유 인덱스(UX_Character_Name_Active)의 위반(2601)으로 판정한다.
    // 조회와 INSERT 사이에 다른 계정이 같은 이름을 넣어도 인덱스가 막는다. 그 밖의 오류는 다시 던져 실행 실패로 끝낸다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SET NOCOUNT ON;
        SET XACT_ABORT ON;

        DECLARE @account_id BIGINT = (?);
        DECLARE @character_id BIGINT = (?);
        DECLARE @class_id INT = (?);
        DECLARE @name NVARCHAR(32) = (?);
        DECLARE @duplicate_name_error INT = (?);
        DECLARE @no_empty_slot_error INT = (?);
        DECLARE @slot_count INT;
        DECLARE @character_count INT;
        DECLARE @result INT = 0;

        BEGIN TRY
            BEGIN TRANSACTION;

            SELECT @slot_count = CharacterSlotCount
            FROM dbo.Account WITH (UPDLOCK, HOLDLOCK)
            WHERE AccountId = @account_id;

            IF @slot_count IS NULL
                THROW 50000, N'계정 행이 없다', 1;

            -- 삭제한 캐릭터는 슬롯을 차지하지 않는다.
            SELECT @character_count = COUNT(*)
            FROM dbo.[Character] WITH (UPDLOCK, HOLDLOCK)
            WHERE AccountId = @account_id
              AND DeletedAt IS NULL;

            IF @character_count >= @slot_count
            BEGIN
                SET @result = @no_empty_slot_error;
                ROLLBACK TRANSACTION;
            END
            ELSE
            BEGIN
                INSERT INTO dbo.[Character] (CharacterId, AccountId, Name, ClassId)
                VALUES (@character_id, @account_id, @name, @class_id);

                COMMIT TRANSACTION;
            END
        END TRY
        BEGIN CATCH
            IF XACT_STATE() <> 0
                ROLLBACK TRANSACTION;

            IF ERROR_NUMBER() <> 2601
                THROW;

            SET @result = @duplicate_name_error;
        END CATCH;

        SELECT @result AS Result;
    )SQL");

    BindObject bindObject(dbBind, accountId, characterId, name, classId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.Fetch() == false)
        throw DBError(__func__, "결과 행이 없다");

    // 거절은 예외가 아니라 결과다. 예외로 나가는 것은 모두 서버 내부 오류다.
    CreateCharacterResult result;
    if (bindObject._result == 0)
    {
        result.characterId = characterId;
        return result;
    }

    // 바인딩한 사유만 거절로 본다. 그 밖의 값은 쿼리가 잘못된 것이다.
    if (ranges::find(bindObject._rejections, bindObject._result) == end(bindObject._rejections))
        throw DBError(__func__, format("쿼리가 모르는 거절 사유 {}를 돌려줬다", bindObject._result));

    result.rejection = static_cast<CharacterRejection>(bindObject._result);
    return result;
}

bool CharacterListDAO::DeleteCharacter(DBConnection& conn, int64 accountId, int64 characterId)
{
    const int PARAMS = 2;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int64 accountId)
            : _characterId(characterId), _accountId(accountId)
        {
            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _accountId);
        }

        //~ Params
        int64 _characterId;
        int64 _accountId;
    };


    // 삭제는 삭제 시각만 적는다. 아이템과 재화 행은 남긴다.
    // AccountId를 함께 대조한다. CharacterId는 클라이언트 값이라, 빼면 다른 계정의 캐릭터를 지울 수 있다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        UPDATE dbo.[Character]
        SET DeletedAt = SYSUTCDATETIME()
        WHERE CharacterId = (?)
          AND AccountId = (?)
          AND DeletedAt IS NULL
    )SQL");

    BindObject bindObject(dbBind, characterId, accountId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    // 갱신한 행이 없으면 이 계정의 캐릭터가 아니거나 이미 지운 캐릭터다.
    return conn.GetRowCount() > 0;
}

bool CharacterListDAO::OwnsCharacter(DBConnection& conn, int64 accountId, int64 characterId)
{
    const int PARAMS = 2;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int64 accountId)
            : _characterId(characterId), _accountId(accountId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _accountId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _count);
        }

        //~ Params
        int64 _characterId;
        int64 _accountId;

        //~ Cols
        int32 _count = 0;
    };


    // AccountId를 함께 대조한다. CharacterId는 클라이언트 값이라, 빼면 다른 계정의 캐릭터로 입장할 수 있다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SELECT COUNT(*)
        FROM dbo.[Character]
        WHERE CharacterId = (?)
          AND AccountId = (?)
          AND DeletedAt IS NULL
    )SQL");

    BindObject bindObject(dbBind, characterId, accountId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.Fetch() == false)
        throw DBError(__func__, "결과 행이 없다");

    return bindObject._count > 0;
}
