#include "pch.h"
#include "CharacterListDAO.h"
#include "DAOCommon.h"
#include "EncodingConverter.h"
#include "CharacterCreation.h"

/*-------------------------
    CharacterListDAO
--------------------------*/

void CharacterListDAO::LoadCharacterList(SessionRef session, int64 userId)
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

        /* Params */
        int64 _userId;

        /* Cols */
        int64 _characterId;
        int32 _classId;
        WCHAR _characterName[100];
        int16 _level;
    };
    
    DBConnectionGuard dbConn;
    Protocol::S_LOGIN pkt;

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(                             
            SELECT character_id, class_id, character_name, level
            FROM [dbo].[Characters]
            WHERE user_id = (?)
            ORDER BY created_at
        )SQL");

        BindObject bindObject(dbBind, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        int32 records = 0;
        while(dbConn->Fetch())
        {
            records++;

            Protocol::CharacterOverview* character = pkt.add_characters();

            character->set_character_id(bindObject._characterId);
            character->set_class_((Protocol::CharacterClass)bindObject._classId);
            character->set_name(EncodingConverter::WCharToString(bindObject._characterName));
            character->set_level(bindObject._level);
        }

        // 클라이언트는 이 값으로 슬롯을 보여 주고 생성 버튼을 막는다. CreateCharacter가 막는 한도와 같은 값이다.
        pkt.set_character_slot_count(CharacterCreation::DEFAULT_CHARACTER_SLOT_COUNT);
        pkt.set_success(true);
    }
    catch (DBCustomError dbError)
    {
        PrintDBErrorLog(dbError);
        pkt.Clear();
        pkt.set_success(false);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(LoadCharacterList): " << err.what() << endl;
        pkt.Clear();
        pkt.set_success(false);
    }
    
    // 패킷 전송
    SEND_PACKET(pkt)
}

void CharacterListDAO::CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId)
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
            const DataTable* classLevelTable = Gamedata::FindClassLevelTable(_classId);
            if (classLevelTable == nullptr)
                throw DBCustomError::UNKNOWN_CHARACTER_CLASS;

            const DataTable& classLevelDataTable = *classLevelTable;
            const int32 level = 1; // 캐릭터 생성 시 초기 레벨은 1.
            const Json& levelData = classLevelDataTable.at(level);
            _curHp = levelData.at(JsonProperty::LevelTable::MaxHp);
            _curMp = levelData.at(JsonProperty::LevelTable::MaxMp);
            _curPhysicalAttack = levelData.at(JsonProperty::LevelTable::PhysicalAttack);
            _curMagicalAttack = levelData.at(JsonProperty::LevelTable::MagicalAttack);
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

        /* Params */
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

        /* Cols */
        int64 _characterId;
    };

    DBConnectionGuard dbConn;
    Protocol::S_CREATE_CHARACTER createCharacterPkt;

    try
    {
        // 계정에 빈 슬롯이 있고 이름이 중복이 아니면 INSERT한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
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

        if (dbConn->Fetch() == false)
            throw DBCustomError::SQL_FETCH_FAIL;

        // character_id는 identity라 양수다. 음수면 쿼리가 거절한 것이고, 부호를 뒤집으면 사유가 된다.
        // 거절을 예외로 던지지 않으므로 catch에 오는 것은 모두 서버 내부 오류다.
        if (bindObject._characterId < 0)
        {
            createCharacterPkt.set_success(false);

            // 바인딩한 사유만 화면에 보여 준다. 그 밖의 음수는 쿼리가 잘못된 것이므로 내부 문구를 내보내지 않는다.
            const int64 code = -bindObject._characterId;
            if (ranges::find(bindObject._rejections, code) != end(bindObject._rejections))
            {
                const DBCustomError rejection = static_cast<DBCustomError>(code);
                PrintDBErrorLog(rejection);
                createCharacterPkt.set_cause(EncodingConverter::WCharToString(DBErrorCauseMappings.at(rejection).c_str()));
            }
            else
            {
                cerr << "Unexpected Rejection(CreateCharacter): " << code << endl;
                createCharacterPkt.set_cause("서버 내부 오류");
            }
        }
        else
        {
            createCharacterPkt.set_success(true);
            createCharacterPkt.set_character_id(bindObject._characterId);
        }
    }
    catch (DBCustomError dbError)
    {
        PrintDBErrorLog(dbError);

        createCharacterPkt.Clear();
        createCharacterPkt.set_success(false);
        createCharacterPkt.set_cause("서버 내부 오류");
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        createCharacterPkt.Clear();
        createCharacterPkt.set_success(false);
        createCharacterPkt.set_cause("알 수 없는 오류");
    }

    // 패킷 전송
    SEND_PACKET(createCharacterPkt)
}

void CharacterListDAO::DeleteCharacter(SessionRef session, int64 characterId)
{
    cout << "DeleteCharacter!" << endl;

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

        /* Params */
        int64 _characterId;
        int64 _userId;
    };

    DBConnectionGuard dbConn;
    Protocol::S_DELETE_CHARACTER deleteCharacterPkt;

    try
    {
        // 캐릭터 삭제. user_id를 함께 대조하므로 이 계정의 캐릭터만 지운다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
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

        GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
        BindObject bindObject(dbBind, characterId, gameSession->_userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (dbConn->GetRowCount() <= 0)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        deleteCharacterPkt.set_success(true);
        deleteCharacterPkt.set_character_id(characterId);
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        deleteCharacterPkt.Clear();
        deleteCharacterPkt.set_success(false);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        deleteCharacterPkt.Clear();
        deleteCharacterPkt.set_success(false);
    }

    // 패킷 전송
    SEND_PACKET(deleteCharacterPkt)
}
