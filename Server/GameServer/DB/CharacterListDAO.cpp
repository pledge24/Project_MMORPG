#include "Core/pch.h"
#include "DB/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Game/Characters/CharacterCreation.h"

void CharacterListDAO::LoadCharacterList(DBConnection& conn, int64 accountId, OUT vector<Protocol::CharacterOverview>& characters)
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
        WCHAR _name[CharacterCreation::MAX_NAME_LENGTH + 1] = {};
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
        Protocol::CharacterOverview* character = &characters.emplace_back();

        character->set_character_id(bindObject._characterId);
        character->set_class_((Protocol::CharacterClass)bindObject._classId);
        character->set_name(EncodingConverter::WCharToString(bindObject._name));
        character->set_level(bindObject._level);
    }
}

CreateCharacterResult CharacterListDAO::CreateCharacter(DBConnection& conn, int64 accountId, int64 characterId, const Protocol::CharacterOverview& character)
{
    const int PARAMS = 15;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 accountId, int64 characterId, const Protocol::CharacterOverview& character)
            : _accountId(accountId), _characterId(characterId), _classId(character.class_()),
              _name(EncodingConverter::StringToWString(character.name()))
        {
            // 핸들러가 CharacterCreation::Validate로 거른다. 그래도 표에 없는 직업이 오면 시작 HP를 모르는 캐릭터를 넣지 않도록 실패로 끝낸다.
            const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_classId);
            const int32 level = 1; // 캐릭터 생성 시 초기 레벨은 1.
            const LevelTemplate* levelTemplate = classLevelTable != nullptr ? classLevelTable->Find(level) : nullptr;
            if (levelTemplate == nullptr)
                throw DBError("CreateCharacter", "레벨 표에 없는 직업");

            // 새 캐릭터는 시작 맵의 시작 위치에서, HP와 MP가 가득 찬 채로, 시작 골드를 들고 시작한다.
            const CharacterCreationTemplate& creation = Gamedata::GetCharacterCreation();
            _mapId = creation.startMapId;
            _posX = creation.startPos.x;
            _posY = creation.startPos.y;
            _posZ = creation.startPos.z;
            _direction = creation.startYaw;
            _currentHp = levelTemplate->maxHp;
            _currentMp = levelTemplate->maxMp;
            _goldCurrencyId = Gamedata::GetGoldCurrencyId();
            _startGold = creation.startGold;

            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _accountId);
            dbBind.BindParam(1, _characterId);
            dbBind.BindParam(2, _classId);
            dbBind.BindParam(3, _name.c_str());
            dbBind.BindParam(4, _mapId);
            dbBind.BindParam(5, _posX);
            dbBind.BindParam(6, _posY);
            dbBind.BindParam(7, _posZ);
            dbBind.BindParam(8, _direction);
            dbBind.BindParam(9, _currentHp);
            dbBind.BindParam(10, _currentMp);
            dbBind.BindParam(11, _goldCurrencyId);
            dbBind.BindParam(12, _startGold);
            dbBind.BindParam(13, _rejections[0]);
            dbBind.BindParam(14, _rejections[1]);
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
        int32 _mapId = 0;
        float _posX = 0.f;
        float _posY = 0.f;
        float _posZ = 0.f;
        float _direction = 0.f;
        int32 _currentHp = 0;
        int32 _currentMp = 0;
        int32 _goldCurrencyId = 0;
        int64 _startGold = 0;
        // 순서는 쿼리의 @duplicate_name_error, @no_empty_slot_error 순서와 맞아야 한다. 어긋나면 거절 사유가 뒤바뀐다.
        int32 _rejections[2] = { static_cast<int32>(CharacterRejection::DUPLICATE_NAME), static_cast<int32>(CharacterRejection::NO_EMPTY_SLOT) };

        //~ Cols
        int32 _result = -1;
    };


    // 세 행을 한 트랜잭션에 넣는다. XACT_ABORT가 켜져 있어 어느 문장이 실패해도 트랜잭션 전체가 되돌려진다.
    // 계정 행을 잠가 같은 계정의 생성 요청을 한 줄로 세운다. 겹쳐 와도 슬롯 한도를 넘지 않는다.
    // 이름 중복은 미리 조회하지 않고 고유 인덱스(UX_Character_Name_Active)의 위반(2601)으로 판정한다.
    // 조회와 INSERT 사이에 다른 계정이 같은 이름을 넣어도 인덱스가 막는다. 그 밖의 오류는 다시 던져 실행 실패로 끝낸다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SET NOCOUNT ON;
        SET XACT_ABORT ON;

        DECLARE @account_id BIGINT = (?);
        DECLARE @character_id BIGINT = (?);
        DECLARE @class_id INT = (?);
        DECLARE @name NVARCHAR(32) = (?);
        DECLARE @map_id INT = (?);
        DECLARE @pos_x REAL = (?);
        DECLARE @pos_y REAL = (?);
        DECLARE @pos_z REAL = (?);
        DECLARE @direction REAL = (?);
        DECLARE @current_hp INT = (?);
        DECLARE @current_mp INT = (?);
        DECLARE @gold_currency_id INT = (?);
        DECLARE @start_gold BIGINT = (?);
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

                INSERT INTO dbo.CharacterState (CharacterId, MapId, PosX, PosY, PosZ, Direction, CurrentHp, CurrentMp)
                VALUES (@character_id, @map_id, @pos_x, @pos_y, @pos_z, @direction, @current_hp, @current_mp);

                INSERT INTO dbo.CharacterCurrency (CharacterId, CurrencyId, Amount)
                VALUES (@character_id, @gold_currency_id, @start_gold);

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

    BindObject bindObject(dbBind, accountId, characterId, character);

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
