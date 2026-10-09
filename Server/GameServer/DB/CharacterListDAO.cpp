#include "Core/pch.h"
#include "DB/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Game/Characters/CharacterCreation.h"

bool CharacterListDAO::LoadCharacterList(DBConnection& conn, int64 userId, OUT vector<Protocol::CharacterOverview>& characters)
{
    const int PARAMS = 1;
    const int COLS = 4;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 userId) : _userId(userId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _userId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _characterId);
            dbBind.BindCol(1, _classId);
            dbBind.BindCol(2, _characterName);
            dbBind.BindCol(3, _level);
        }

        //~ Params
        int64 _userId;

        //~ Cols
        int64 _characterId;
        int32 _classId;
        WCHAR _characterName[100];
        int16 _level;
    };
    

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(                             
            SELECT character_id, class_id, character_name, level
            FROM [dbo].[Characters]
            WHERE user_id = (?)
            ORDER BY created_at
        )SQL");

        BindObject bindObject(dbBind, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        while(conn.Fetch())
        {
            Protocol::CharacterOverview* character = &characters.emplace_back();

            character->set_character_id(bindObject._characterId);
            character->set_class_((Protocol::CharacterClass)bindObject._classId);
            character->set_name(EncodingConverter::WCharToString(bindObject._characterName));
            character->set_level(bindObject._level);
        }
    }
    catch (DBCustomError dbError)
    {
        PrintDBErrorLog(dbError);
        characters.clear();
        return false;
    }
    catch (exception& err)
    {
        GLogger->Error("Unexpected Error(LoadCharacterList): {}", err.what());
        characters.clear();
        return false;
    }

    return true;
}

bool CharacterListDAO::CreateCharacter(DBConnection& conn, const Protocol::CharacterOverview& character, int64 userId, OUT CreateCharacterResult& result)
{
    const int PARAMS = 10;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
            : _userId(userId), _classId(character.class_()), _name(EncodingConverter::StringToWString(character.name())),
              _slotCount(CharacterCreation::DEFAULT_CHARACTER_SLOT_COUNT)
        {
            // 핸들러가 CharacterCreation::Validate로 거른다. 그래도 표에 없는 직업이 오면 끼워 넣지 않고 실패로 끝낸다.
            const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(_classId);
            const int32 level = 1; // 캐릭터 생성 시 초기 레벨은 1.
            const LevelTemplate* levelTemplate = classLevelTable != nullptr ? classLevelTable->Find(level) : nullptr;
            if (levelTemplate == nullptr)
                throw DBCustomError::UNKNOWN_CHARACTER_CLASS;

            _curHp = levelTemplate->maxHp;
            _curMp = levelTemplate->maxMp;
            _curPhysicalAttack = levelTemplate->physicalAttack;
            _curMagicalAttack = levelTemplate->magicalAttack;
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _userId);
            dbBind.BindParam(1, _classId);
            dbBind.BindParam(2, _name.c_str());
            dbBind.BindParam(3, _slotCount);
            dbBind.BindParam(4, _rejections[0]);
            dbBind.BindParam(5, _rejections[1]);
            dbBind.BindParam(6, _curHp);
            dbBind.BindParam(7, _curMp);
            dbBind.BindParam(8, _curPhysicalAttack);
            dbBind.BindParam(9, _curMagicalAttack);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _characterId);
        }

        //~ Params
        int64 _userId;
        int32 _classId;
        wstring _name;
        int32 _slotCount;
        // 쿼리가 거절할 때 character_id 자리에 부호를 뒤집어 돌려주는 사유다. 화면에 그대로 보여 준다.
        // 순서는 쿼리의 @duplicate_name_error, @no_empty_slot_error 순서와 같다.
        int32 _rejections[2] = { DBCustomError::ALREADY_EXISTING_CHARACTER, DBCustomError::NO_EMPTY_CHARACTER_SLOT };
        int32 _curHp = 0;
        int32 _curMp = 0;
        int32 _curPhysicalAttack = 0;
        int32 _curMagicalAttack = 0;

        //~ Cols
        int64 _characterId;
    };


    try
    {
        // 계정에 빈 슬롯이 있고 이름이 중복이 아니면 INSERT한다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            SET NOCOUNT ON;

            BEGIN TRANSACTION;

            DECLARE @existing_character_id BIGINT;
            DECLARE @character_id BIGINT;
            DECLARE @user_id BIGINT = (?);
            DECLARE @class_id INT = (?);
            DECLARE @character_name NVARCHAR(50) = (?);
            DECLARE @slot_count INT = (?);
            DECLARE @duplicate_name_error INT = (?);
            DECLARE @no_empty_slot_error INT = (?);
            DECLARE @character_count INT;

            -- 계정의 캐릭터 수 확인 (트랜잭션 락). 같은 계정의 생성 요청이 겹쳐도 한도를 넘지 않는다
            SELECT @character_count = COUNT(*)
            FROM [dbo].[Characters] WITH(UPDLOCK, HOLDLOCK)
            WHERE user_id = @user_id;

            -- 중복 이름 확인 (트랜잭션 락)
            SELECT @existing_character_id = character_id
            FROM [dbo].[Characters] WITH(UPDLOCK, HOLDLOCK)
            WHERE character_name = @character_name;

            IF @character_count >= @slot_count
            BEGIN
                SELECT -@no_empty_slot_error AS character_id;

                ROLLBACK TRANSACTION;
            END
            ELSE IF @existing_character_id IS NULL
            BEGIN
                -- 1. 캐릭터 기본 정보 삽입 
                INSERT INTO [dbo].[Characters]([user_id], [class_id], [character_name])
                VALUES(@user_id, @class_id, @character_name);
                
                -- 마지막에 삽입된 ID 가져오기(identity로)
                SET @character_id = SCOPE_IDENTITY();
                
                -- 2. 캐릭터 마지막 상태 저장 (기본값)
                INSERT INTO [dbo].[CharactersLastState]([character_id], [cur_hp], [cur_mp], [cur_physical_attack], [cur_magical_attack])
                VALUES(@character_id, (?), (?), (?), (?));
                
                -- 3. 기본 아이템을 추가(보류)
                --INSERT INTO [dbo].[CharactersGearItems]([character_id], [template_id], [is_equipped], [slot_id])
                --VALUES(@character_id, 1005, 1, 1);

                -- 4. 결과셋으로 반환
                SELECT @character_id AS character_id;
                
                COMMIT TRANSACTION;
            END
            ELSE
            BEGIN
                SELECT -@duplicate_name_error AS character_id;

                ROLLBACK TRANSACTION;
            END
        )SQL");
         
        BindObject bindObject(dbBind, character, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (conn.Fetch() == false)
            throw DBCustomError::SQL_FETCH_FAIL;

        // character_id는 identity라 양수다. 음수면 쿼리가 거절한 것이고, 부호를 뒤집으면 사유가 된다.
        // 거절을 예외로 던지지 않으므로 catch에 오는 것은 모두 서버 내부 오류다.
        if (bindObject._characterId < 0)
        {
            // 바인딩한 사유만 화면에 보여 준다. 그 밖의 음수는 쿼리가 잘못된 것이므로 내부 오류로 본다.
            const int64 code = -bindObject._characterId;
            if (ranges::find(bindObject._rejections, code) == end(bindObject._rejections))
            {
                GLogger->Error("Unexpected Rejection(CreateCharacter): {}", code);
                return false;
            }

            const DBCustomError rejection = static_cast<DBCustomError>(code);
            PrintDBErrorLog(rejection);
            result.rejection = EncodingConverter::WCharToString(DBErrorCauseMappings.at(rejection).c_str());
        }
        else
        {
            result.characterId = bindObject._characterId;
        }
    }
    catch (DBCustomError dbError)
    {
        PrintDBErrorLog(dbError);
        return false;
    }
    catch (exception& err)
    {
        GLogger->Error("Unexpected Error(CreateCharacter): {}", err.what());
        return false;
    }

    return true;
}

bool CharacterListDAO::DeleteCharacter(DBConnection& conn, int64 userId, int64 characterId)
{
    const int PARAMS = 2;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int64 userId) 
            : _characterId(characterId), _userId(userId)
        {
            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _userId);
        }

        //~ Params
        int64 _characterId;
        int64 _userId;
    };


    try
    {
        // 캐릭터 삭제. user_id를 함께 대조하므로 이 계정의 캐릭터만 지운다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            BEGIN TRANSACTION;

            DECLARE @character_id BIGINT;
            DECLARE @user_id BIGINT;
            SET @character_id = (?);
            SET @user_id = (?);

            IF EXISTS (
                SELECT 1 
                FROM [dbo].[Characters] 
                WHERE [character_id] = @character_id 
                  AND [user_id] = @user_id
            )
            BEGIN 
                DELETE FROM [dbo].[Characters]
                WHERE [character_id] = @character_id
                  AND [user_id] = @user_id;

                COMMIT TRANSACTION;
            END
            ELSE
            BEGIN
                ROLLBACK TRANSACTION;
            END
        )SQL");

        BindObject bindObject(dbBind, characterId, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (conn.GetRowCount() <= 0)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }
    catch (exception& err)
    {
        GLogger->Error("Unexpected Error(DeleteCharacter): {}", err.what());
        return false;
    }

    return true;
}
