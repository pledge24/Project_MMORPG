#include "pch.h"
#include "DBRequestFunctions.h"
#include "EncodingConverter.h"
#include "Player.h"
#include "Global.h"
#include "Inventory.h"
#include "EquippedGear.h"

enum DBCustomError
{
    NONE = 24000,
    SQL_EXECUTE_FAIL = 24001,
    SQL_FETCH_FAIL = 24002,
    ALREADY_EXISTING_CHARACTER = 24003,
    SQL_MISMATCHED_GET_ROW_COUNT = 24004,
    SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE = 24005
};

const unordered_map<DBCustomError, wstring> DBErrorCauseMappings =
{
    {NONE, L""},
    {SQL_EXECUTE_FAIL, L"Execute() false 반환"},
    {SQL_FETCH_FAIL, L"Fetch() false 반환"},
    {ALREADY_EXISTING_CHARACTER, L"이미 존재하는 캐릭터입니다."},
    {SQL_MISMATCHED_GET_ROW_COUNT, L"GetRowCount() 불일치 발생"},
    {SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE, L"파라미터 배열 처리 행 수 불일치 발생"},
};

void PrintDBErrorLog(const DBCustomError error)
{
    wcerr << L"오류 발생: " << error << L"(" << DBErrorCauseMappings.at(error) << L")" << endl;
}

/*-------------------------
    DBRequestFunctions
--------------------------*/

void DBRequestFunctions::LoadUserCharactersData(SessionRef session, int64 userId)
{
    cout << "GetUserCharactersData!" << endl;

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
    
    DBConnection* dbConn = GDBConnectionPool->Pop();
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

            wcout << "characterId: " << bindObject._characterId << endl;
            wcout << "classId: " << bindObject._classId << endl;
            wcout << "characterName: " << bindObject._characterName << endl;
            wcout << "level: " << bindObject._level << endl;

            character->set_character_id(bindObject._characterId);
            character->set_class_((Protocol::CharacterClass)bindObject._classId);
            character->set_name(EncodingConverter::WCharToString(bindObject._characterName));
            character->set_level(bindObject._level);
        }

        wcout << L"캐릭터 개수: " << records << endl;

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
        cerr << "Unexpected Error(GetCharacterData): " << err.what() << endl;
        pkt.Clear();
        pkt.set_success(false);
    }
    
    // 패킷 전송
    SEND_PACKET(pkt);

    GDBConnectionPool->Push(dbConn);   
}

void DBRequestFunctions::CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId)
{
    cout << "CreateCharacter!" << endl;

    const int PARAMS = 7;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const Protocol::CharacterOverview& character, int64 userId)
            : _userId(userId), _classId(character.class_()), _name(EncodingConverter::StringToWString(character.name()))
        {
            unordered_map<int32, Json>& classLevelDataTable = (*Gamedata::ClassLevelDataTableMappings[_classId]);
            const int32 level = 1; // 캐릭터 생성 시 초기 레벨은 1.
            _curHp = classLevelDataTable[level]["maxHp"];
            _curMp = classLevelDataTable[level]["maxMp"];
            _curPhysicalAttack = classLevelDataTable[level]["physicalAttack"];
            _curMagicalAttack = classLevelDataTable[level]["magicalAttack"];
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _userId);
            dbBind.BindParam(1, _classId);
            dbBind.BindParam(2, _name.c_str());
            dbBind.BindParam(3, _curHp);
            dbBind.BindParam(4, _curMp);
            dbBind.BindParam(5, _curPhysicalAttack);
            dbBind.BindParam(6, _curMagicalAttack);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _characterId);
        }

        /* Params */
        int64 _userId;
        int32 _classId;
        wstring _name;
        int32 _curHp = 0;
        int32 _curMp = 0;
        int32 _curPhysicalAttack = 0;
        int32 _curMagicalAttack = 0;

        /* Cols */
        int64 _characterId;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();
    Protocol::S_CREATE_CHARACTER pkt;

    try
    {
        // 전달받은 이름이 중복인지 확인하고, 아니라면 INSERT한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SET NOCOUNT ON;

            BEGIN TRANSACTION; 
            
            DECLARE @existing_character_id BIGINT; 
            DECLARE @character_id BIGINT; 
            DECLARE @user_id BIGINT = (?); 
            DECLARE @class_id INT = (?); 
            DECLARE @character_name NVARCHAR(50) = (?); 
            
            -- 중복 이름 확인 (트랜잭션 락)
            SELECT @existing_character_id = character_id 
            FROM [dbo].[Characters] WITH(UPDLOCK, HOLDLOCK) 
            WHERE character_name = @character_name; 
            
            IF @existing_character_id IS NULL
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
                SELECT -1 as character_id;

                ROLLBACK TRANSACTION;
            END
        )SQL");
         
        BindObject bindObject(dbBind, character, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (dbConn->Fetch() == false)
            throw DBCustomError::SQL_FETCH_FAIL;

        if (bindObject._characterId == -1)
        {
            throw DBCustomError::ALREADY_EXISTING_CHARACTER;
        }

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        pkt.set_success(true);
        pkt.set_character_id(bindObject._characterId);
        SEND_PACKET(pkt);
    }
    catch (DBCustomError dbError)
    {
        PrintDBErrorLog(dbError);

        pkt.Clear();
        pkt.set_success(false);
        if (dbError == DBCustomError::ALREADY_EXISTING_CHARACTER)
            pkt.set_cause(EncodingConverter::WCharToString(DBErrorCauseMappings.at(dbError).c_str()));
        else
            pkt.set_cause("서버 내부 오류");
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        pkt.Clear();
        pkt.set_success(false);
        pkt.set_cause("알 수 없는 오류");
    }

    // 패킷 전송
    SEND_PACKET(pkt);

    GDBConnectionPool->Push(dbConn);
}

void DBRequestFunctions::DeleteCharacter(SessionRef session, int64 characterId)
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

    DBConnection* dbConn = GDBConnectionPool->Pop();
    Protocol::S_DELETE_CHARACTER pkt;

    try
    {
        // 캐릭터 삭제
        // TODO: 다른 유저가 내 캐릭터를 지워버리지 못하도록 해야함
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            BEGIN TRANSACTION;
            
            DECLARE @character_id BIGINT;
            DECLARE @user_id BIGINT;
            SET @character_id = (?);
            SET @user_id = (?);
            
            IF EXISTS (SELECT 1 FROM [dbo].[Characters] WHERE [character_id] = @character_id) AND [user_id] = @user_id
            BEGIN 
                DELETE FROM [dbo].[Characters]
                WHERE [character_id] = @character_id
                COMMIT TRANSACTION;
            END
            ELSE
            BEGIN
                ROLLBACK TRANSACTION;
            END
        )SQL");

        GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
        BindObject bindObject(dbBind, characterId, gameSession->userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (dbConn->GetRowCount() <= 0)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        pkt.set_success(true);
        pkt.set_character_id(characterId);
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        pkt.Clear();
        pkt.set_success(false);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        pkt.Clear();
        pkt.set_success(false);
    }

    // 패킷 전송
    SEND_PACKET(pkt);

    GDBConnectionPool->Push(dbConn);
}

void DBRequestFunctions::LoadAllCharactersData(SessionRef session, int64 characterId)
{
    PlayerRef player = static_pointer_cast<GameSession>(session)->player;

    // 1. 캐릭터 기본 정보 다시 가져오기(이름, 레벨 등 필요)
    if (LoadCharacterData(session, characterId) == false)
    {
        cout << "Error In GetCharacterData" << endl;
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState)가져오기
    if (LoadCharacterLastStateData(session, characterId) == false)
    {
        cout << "Error In GetCharacterLastStateData" << endl;
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 가져오기
    if (LoadAllCharacterItems(session, characterId) == false)
    {
        cout << "Error In GetCharacterInventoryData" << endl;
        return;
    }

    // DB에서 가져온 스펙을 기반으로 최종 스텟 계산
    if (player->Init() == false)
        return;
    
    // 패킷으로 만들어서 클라이언트에게 보낸다.
    Protocol::S_ENTER_GAME pkt;
    pkt.set_success(true);
    pkt.mutable_player()->CopyFrom(*player->objectInfo);
    SEND_PACKET(pkt);
}

void DBRequestFunctions::UpdateAllCharactersData(SessionRef session)
{
    // 1. 캐릭터 기본 정보 업데이트(이름, 레벨 등 필요)
    if (UpdateCharacterData(session) == false)
    {
        wcout << L"캐릭터 기본 정보 업데이트 실패" << endl;
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState) 업데이트
    if (UpdateCharacterLastStateData(session) == false)
    {
        wcout << L"캐릭터 마지막 상태(LastState) 업데이트 실패" << endl;
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 업데이트
    if (UpdateAllCharacterItems(session) == false)
    {
        wcout << L"캐릭터 소유 아이템 정보 업데이트 실패" << endl;
        return;
    }
}

bool DBRequestFunctions::GetMaxItemUID()
{
    const int PARAMS = 0;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind)
        {
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {

            dbBind.BindCol(0, _maxItemUID);
        }

        /* Cols */
        int32 _maxItemUID;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            EXEC GetMaxItemUID;
        )SQL");

        BindObject bindObject(dbBind);

        if (dbBind.Execute() == false)
            throw wstring(L"Execute() 실패");

        if (dbConn->Fetch() == false)
            throw wstring(L"Fetch() 실패");

        GNextItemUID = bindObject._maxItemUID + 1; // itemUid저장
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::LoadCharacterData(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 3;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) : _characterId(characterId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _classId);
            dbBind.BindCol(1, _characterName);
            dbBind.BindCol(2, _level);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int32 _classId;
        WCHAR _characterName[100];
        int16 _level;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT class_id, character_name, level
            FROM [dbo].[Characters]
            WHERE character_id = (?)
            ORDER BY created_at
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;

        dbConn->Fetch();

        Protocol::PlayerInfo* playerInfo = player->playerInfo;

        playerInfo->set_character_id(bindObject._characterId);
        playerInfo->set_class_((Protocol::CharacterClass)bindObject._classId);
        playerInfo->set_name(EncodingConverter::WCharToString(bindObject._characterName));
        playerInfo->set_level(bindObject._level);
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }
    
    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::LoadCharacterLastStateData(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 11;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) 
            : _characterId(characterId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _exp);
            dbBind.BindCol(1, _curHp);
            dbBind.BindCol(2, _curMp);
            dbBind.BindCol(3, _curPhysicalAttack);
            dbBind.BindCol(4, _curMagicalAttack);
            dbBind.BindCol(5, _mapId);
            dbBind.BindCol(6, _posX);
            dbBind.BindCol(7, _posY);
            dbBind.BindCol(8, _posZ);
            dbBind.BindCol(9, _rotYaw);
            dbBind.BindCol(10, _gold);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int64 _exp;
        int32 _curHp;
        int32 _curMp;
        int32 _curPhysicalAttack;
        int32 _curMagicalAttack;
        int32 _mapId;
        float _posX;
        float _posY;
        float _posZ;
        float _rotYaw;
        int64 _gold;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 마지막 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT exp, cur_hp, cur_mp, cur_physical_attack, cur_magical_attack, map_id, pos_x, pos_y, pos_z, rot_yaw, gold
            FROM [dbo].[CharactersLastState] 
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            return false;

        if (dbConn->Fetch() == false)
            return false;

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::PlayerInfo* playerInfo = player->playerInfo;
        Protocol::PosInfo* posInfo = player->posInfo;

        // ==성장 및 스텟 관련==
        playerInfo->set_cur_exp(bindObject._exp);
        DataTable& classLevelDataTable = *Gamedata::ClassLevelDataTableMappings[playerInfo->class_()];
        uint64 maxExp = classLevelDataTable[playerInfo->level()]["expRequirement"];
        playerInfo->set_max_exp(maxExp);

        // 현재 Hp
        Protocol::StatInfo* statInfo = player->statInfo;
        statInfo->set_hp(bindObject._curHp);
        statInfo->set_mp(bindObject._curMp);
        statInfo->set_physical_attack(bindObject._curPhysicalAttack);
        statInfo->set_magical_attack(bindObject._curMagicalAttack);

        // 위치 설정
        posInfo->set_map_id(bindObject._mapId);
        posInfo->set_x(bindObject._posX);
        posInfo->set_y(bindObject._posY);
        posInfo->set_z(bindObject._posZ);
        posInfo->set_yaw(bindObject._rotYaw);

        // ==플레이어 골드 설정==
        playerInfo->set_gold(bindObject._gold);
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::LoadAllCharacterItems(SessionRef session, int64 characterId)
{
    // 1. 캐릭터 장비 아이템 가져오기
    if (LoadCharactersGearItems(session, characterId) == false)
    {
        cout << "Error In LoadCharactersGearItems" << endl;
        return false;
    }

    // 2. 캐릭터 소비 아이템 가져오기
    if (LoadCharactersConsumableItems(session, characterId) == false)
    {
        cout << "Error In LoadCharactersConsumableItems" << endl;
        return false;
    }

    // 3. 캐릭터 기타 아이템 가져오기
    if (LoadCharactersMiscItems(session, characterId) == false)
    {
        cout << "Error In LoadCharactersMiscItems" << endl;
        return false;
    }

    return true;
}

bool DBRequestFunctions::LoadCharactersGearItems(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 8;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) : _characterId(characterId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _itemUid);
            dbBind.BindCol(1, _templateId);
            dbBind.BindCol(2, _isEquipped);
            dbBind.BindCol(3, _slotId);
            dbBind.BindCol(4, _enhance);
            dbBind.BindCol(5, _durability);
            dbBind.BindCol(6, _additionalPhysicalAttack);
            dbBind.BindCol(7, _additionalMagicalAttack);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int64 _itemUid;
        int32 _templateId;
        bool _isEquipped;
        int32 _slotId;
        int32 _enhance;
        int32 _durability;
        int32 _additionalPhysicalAttack;
        int32 _additionalMagicalAttack;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 장비 아이템 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT item_uid, template_id, slot_id, enhance, durability, additional_physical_attack, additional_magical_attack
            FROM [dbo].[CharactersGearItems]
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;

        while (dbConn->Fetch())
        {
            Protocol::Item item;
            Protocol::GearInfo* gearInfo = item.mutable_gearinfo();

            item.set_template_id(bindObject._templateId);
            item.set_item_uid(bindObject._itemUid);

            gearInfo->set_enhance_level(bindObject._enhance);
            gearInfo->set_durability(bindObject._durability);
            gearInfo->set_additional_physical_attack(bindObject._additionalMagicalAttack);
            gearInfo->set_additional_magical_attack(bindObject._additionalMagicalAttack);

            if (bindObject._isEquipped == false)
                player->inventory->addItem(nullptr, item, 1, bindObject._slotId);
            else
                player->equippedGear->EquipGear(nullptr, nullptr, item, bindObject._slotId);
        }
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::LoadCharactersConsumableItems(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 3;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) : _characterId(characterId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _slotId);
            dbBind.BindCol(1, _templateId);
            dbBind.BindCol(2, _count);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int32 _slotId;
        int32 _templateId;
        int32 _count;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 소비 아이템 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT slot_id, template_id, count
            FROM [dbo].[CharactersConsumableItems]
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;

        while (dbConn->Fetch())
        {
            Protocol::Item item;

            item.set_template_id(bindObject._templateId);
            player->inventory->addItem(nullptr, item, bindObject._count, bindObject._slotId);
        }
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::LoadCharactersMiscItems(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 3;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) : _characterId(characterId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _slotId);
            dbBind.BindCol(1, _templateId);
            dbBind.BindCol(2, _count);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int32 _slotId;
        int32 _templateId;
        int32 _count;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 기타 아이템 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT slot_id, template_id, count
            FROM [dbo].[CharactersMiscItems]
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;

        while (dbConn->Fetch())
        {
            Protocol::Item item;

            item.set_template_id(bindObject._templateId);
            player->inventory->addItem(nullptr, item, bindObject._count, bindObject._slotId);
        }
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::UpdateCharacterData(SessionRef session)
{
    const int PARAMS = 2;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int16 level) : _characterId(characterId), _level(level)
        {
            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _level);
        }

        /* Params */
        int64 _characterId;
        int16 _level;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 갱신한다(지금은 레벨만 갱신).
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            UPDATE [dbo].[Characters]
            SET level = (?)
            WHERE character_id = (?)
        )SQL");

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::ObjectInfo* objectInfo = static_pointer_cast<Object>(player)->objectInfo;
        Protocol::PlayerInfo* playerInfo = objectInfo->mutable_player_info();

        BindObject bindObject(dbBind, playerInfo->character_id(), playerInfo->level());

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (dbConn->GetRowCount() != 1)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::UpdateCharacterLastStateData(SessionRef session)
{
    const int PARAMS = 12;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, Protocol::ObjectInfo* objectInfo)
        {
            const Protocol::PlayerInfo& playerInfo = objectInfo->player_info();
            const Protocol::PosInfo& posInfo = objectInfo->pos_info();

            _exp = playerInfo.cur_exp();
            _curHp = playerInfo.stat_info().hp();
            _curMp = playerInfo.stat_info().mp();
            _curPhysicalAttack = playerInfo.stat_info().physical_attack();
            _curMagicalAttack = playerInfo.stat_info().magical_attack();
            _mapId = 1;
            _posX = posInfo.x();
            _posY = posInfo.y();
            _posZ = posInfo.z();
            _rotYaw = posInfo.yaw();
            _gold = playerInfo.gold();
            _characterId = playerInfo.character_id();

            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _exp);
            dbBind.BindParam(1, _curHp);
            dbBind.BindParam(2, _curMp);
            dbBind.BindParam(3, _curPhysicalAttack);
            dbBind.BindParam(4, _curMagicalAttack);
            dbBind.BindParam(5, _mapId);
            dbBind.BindParam(6, _posX);
            dbBind.BindParam(7, _posY);
            dbBind.BindParam(8, _posZ);
            dbBind.BindParam(9, _rotYaw);
            dbBind.BindParam(10, _gold);
            dbBind.BindParam(11, _characterId);
        }

        /* Params */
        int64 _exp;
        int32 _curHp;
        int32 _curMp;
        int32 _curPhysicalAttack;
        int32 _curMagicalAttack;
        int32 _mapId;
        float _posX;
        float _posY;
        float _posZ;
        float _rotYaw;
        int64 _gold;
        int64 _characterId;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 유저의 마지막 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            UPDATE [dbo].[CharactersLastState]
            SET exp = (?), cur_hp = (?), cur_mp = (?), cur_physical_attack = (?), cur_magical_attack = (?), map_id = (?), pos_x = (?), pos_y = (?), pos_z = (?), rot_yaw = (?), gold = (?)
            WHERE character_id = (?)
        )SQL");

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::ObjectInfo* objectInfo = static_pointer_cast<Object>(player)->objectInfo;

        BindObject bindObject(dbBind, objectInfo);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (dbConn->GetRowCount() != 1)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::UpdateAllCharacterItems(SessionRef session)
{
    // 1. 캐릭터 장비 아이템 가져오기
    if (UpdateCharactersGearItems(session) == false)
    {
        wcout << L"캐릭터 장비 아이템 갱신 실패" << endl;
        return false;
    }

    // 2. 캐릭터 소비 아이템 가져오기
    if (UpdateCharactersConsumableItems(session) == false)
    {
        wcout << L"캐릭터 소비 아이템 갱신 실패" << endl;
        return false;
    }

    // 3. 캐릭터 기타 아이템 가져오기
    if (UpdateCharactersMiscItems(session) == false)
    {
        wcout << L"캐릭터 기타 아이템 갱신 실패" << endl;
        return false;
    }

    return true;
}

bool DBRequestFunctions::UpdateCharactersGearItems(SessionRef session)
{
    const int PARAMS = 9;
    const int COLS = 0;
    const int MAX_ROWS = 100;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, Protocol::ObjectInfo* objectInfo, PlayerRef player, int32& rows)
        {
            const Protocol::PlayerInfo& playerInfo = objectInfo->player_info();
            const Protocol::Inventory& inven = playerInfo.inventory();
            vector<bool>& gearDirtyFlags = player->inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_GEAR);

            // 인벤에 들어있는 장비
            for (int i = 0; i < inven.gear_size(); i++)
            {
                if (gearDirtyFlags[i] == true)
                {
                    const Protocol::Slot slot = inven.gear().Get(i);
                    _slotId[rows] = slot.slot_id();
                    _itemUid[rows] = slot.item().item_uid();
                    _templateId[rows] = slot.item().template_id();
                    _isEquipped[rows] = false;
                    _enhance[rows] = slot.item().gearinfo().enhance_level();
                    _durability[rows] = slot.item().gearinfo().durability();
                    _additionalPhysicalAttack[rows] = slot.item().gearinfo().additional_physical_attack();
                    _additionalMagicalAttack[rows] = slot.item().gearinfo().additional_magical_attack();

                    rows++;
                }
            }

            // 장착 중인 장비
            vector<bool>& equippedGearDirtyFlags = player->equippedGear->GetDirtyFlags();
            for (int i = 0; i < playerInfo.equipped_gear_size(); i++)
            {
                if (equippedGearDirtyFlags[i] == true)
                {
                    const Protocol::Slot slot = playerInfo.equipped_gear().Get(i);
                    _slotId[rows] = slot.slot_id();
                    _itemUid[rows] = slot.item().item_uid();
                    _templateId[rows] = slot.item().template_id();
                    _isEquipped[rows] = true;
                    _enhance[rows] = slot.item().gearinfo().enhance_level();
                    _durability[rows] = slot.item().gearinfo().durability();
                    _additionalPhysicalAttack[rows] = slot.item().gearinfo().additional_physical_attack();
                    _additionalMagicalAttack[rows] = slot.item().gearinfo().additional_magical_attack();

                    rows++;
                }
            }

            BindParam(dbBind, rows);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind, int32 rows)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _slotId, rows);
            dbBind.BindParam(2, _itemUid, rows);
            dbBind.BindParam(3, _templateId, rows);
            dbBind.BindParam(4, _isEquipped, rows);
            dbBind.BindParam(5, _enhance, rows);
            dbBind.BindParam(6, _durability, rows);
            dbBind.BindParam(7, _additionalPhysicalAttack, rows);
            dbBind.BindParam(8, _additionalMagicalAttack, rows);
        }

        /* Params */
        int64 _characterId;
        int32 _slotId[MAX_ROWS];
        int64 _itemUid[MAX_ROWS];
        int32 _templateId[MAX_ROWS];
        bool _isEquipped[MAX_ROWS];
        int32 _enhance[MAX_ROWS];
        int32 _durability[MAX_ROWS];
        int32 _additionalPhysicalAttack[MAX_ROWS];
        int32 _additionalMagicalAttack[MAX_ROWS];
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 장비 아이템 정보를 갱신한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            DECLARE @character_id BIGINT = (?); 

            -- 1. 임시 테이블 생성
            SELECT *
            INTO #TempTable
            FROM [dbo].[CharactersGearItems]
            WHERE 1 = 0;

            -- 2. 임시 테이블에 INSERT
            INSERT INTO #TempTable (character_id, slot_id, item_uid, template_id, is_equipped, enhance, durability, additional_physical_attack, additional_magical_attack)
            VALUES (@character_id, ?, ?, ?, ?, ?, ?, ?, ?)

            -- 3. MERGE 진행
            MERGE INTO [dbo].[CharactersGearItems] AS T
            USING #TempTable AS S
            ON (T.character_id = S.character_id AND T.slot_id = S.slot_id)

            -- 3-1). 둘 다 행이 존재 -> UPDATE
            WHEN MATCHED THEN
                UPDATE SET item_uid = S.item_uid, template_id = S.template_id, is_equipped = S.is_equipped, enhance = S.enhance, durability = S.durability, additional_physical_attack = S.additional_physical_attack, additional_magical_attack = S.additional_magical_attack

            -- 3-2). DB 테이블에 행이 없음 -> INSERT
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (character_id, slot_id, item_uid, template_id, is_equipped, enhance, durability, additional_physical_attack, additional_magical_attack)
                VALUES (S.character_id, S.slot_id, S.tem_uid, S.template_id, S.is_equipped, S.enhance, S.durability, S.additional_physical_attack, S.additional_magical_attack)

            -- 3-3). 임시 테이블에 행이 없음 -> DELETE
            WHEN NOT MATCHED BY SOURCE AND T.character_id = @character_id THEN
                DELETE;

            -- 4. 임시 테이블 삭제
            DROP TABLE #TempItems;
        )SQL");

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::ObjectInfo* objectInfo = static_pointer_cast<Object>(player)->objectInfo;

        int32 rows = 0;
        int32 processedRows = 0;
        BindObject bindObject(dbBind, objectInfo, player, OUT rows);

        // 파마미터 배열 집합 크기 세팅
        dbConn->SetParamSetSize(rows);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (rows != processedRows)
            throw DBCustomError::SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::UpdateCharactersConsumableItems(SessionRef session)
{
    const int PARAMS = 4;
    const int COLS = 0;
    const int MAX_ROWS = 100;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, Protocol::ObjectInfo* objectInfo, PlayerRef player, int32 rows)
        {
            const Protocol::PlayerInfo& playerInfo = objectInfo->player_info();
            const Protocol::Inventory& inven = playerInfo.inventory();
            vector<bool>& consumableDirtyFlags = player->inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_CONSUMABLE);

            // 인벤에 들어있는 장비
            for (int i = 0; i < inven.consumables_size(); i++)
            {
                if (consumableDirtyFlags[i] == true)
                {
                    const Protocol::Slot slot = inven.gear().Get(i);
                    _slotId[rows] = slot.slot_id();
                    _templateId[rows] = slot.item().template_id();
                    _count[rows] = slot.count();

                    rows++;
                }
            }

            BindParam(dbBind, rows);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind, int32 rows)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _slotId, rows);
            dbBind.BindParam(2, _templateId, rows);
            dbBind.BindParam(3, _count, rows);
        }

        /* Params */
        int64 _characterId;
        int32 _slotId[MAX_ROWS];
        int32 _templateId[MAX_ROWS];
        int32 _count[MAX_ROWS];
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 장비 아이템 정보를 갱신한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            DECLARE @character_id BIGINT = (?); 

            -- 1. 임시 테이블 생성
            SELECT *
            INTO #TempTable
            FROM [dbo].[CharactersConsumableItems]
            WHERE 1 = 0;

            -- 2. 임시 테이블에 INSERT
            INSERT INTO #TempTable (character_id, slot_id, template_id, count)
            VALUES (@character_id, ?, ?, ?)

            -- 3. MERGE 진행
            MERGE INTO [dbo].[CharactersConsumableItems] AS T
            USING #TempTable AS S
            ON (T.character_id = S.character_id AND T.slot_id = S.slot_id)

            -- 3-1). 둘 다 행이 존재 -> UPDATE
            WHEN MATCHED THEN
                UPDATE SET template_id = S.template_id, count = S.count

            -- 3-2). DB 테이블에 행이 없음 -> INSERT
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (character_id, slot_id, template_id, count)
                VALUES (S.character_id, S.slot_id, S.template_id, S.count)

            -- 3-3). 임시 테이블에 행이 없음 -> DELETE
            WHEN NOT MATCHED BY SOURCE AND T.character_id = @character_id THEN
                DELETE;

            -- 4. 임시 테이블 삭제
            DROP TABLE #TempItems;
        )SQL");

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::ObjectInfo* objectInfo = static_pointer_cast<Object>(player)->objectInfo;

        int32 rows = 0;
        int32 processedRows = 0;
        BindObject bindObject(dbBind, objectInfo, player, OUT rows);

        dbConn->SetParamSetSize(rows);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (rows != processedRows)
            throw DBCustomError::SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}

bool DBRequestFunctions::UpdateCharactersMiscItems(SessionRef session)
{
    const int PARAMS = 4;
    const int COLS = 0;
    const int MAX_ROWS = 100;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, Protocol::ObjectInfo* objectInfo, PlayerRef player, int32 rows)
        {
            const Protocol::PlayerInfo& playerInfo = objectInfo->player_info();
            const Protocol::Inventory& inven = playerInfo.inventory();
            vector<bool>& miscDirtyFlags = player->inventory->GetDirtyFlags(Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS);

            // 인벤에 들어있는 장비
            for (int i = 0; i < inven.consumables_size(); i++)
            {
                if (miscDirtyFlags[i] == true)
                {
                    const Protocol::Slot slot = inven.gear().Get(i);
                    _slotId[rows] = slot.slot_id();
                    _templateId[rows] = slot.item().template_id();
                    _count[rows] = slot.count();

                    rows++;
                }
            }

            BindParam(dbBind, rows);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind, int32 rows)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _slotId, rows);
            dbBind.BindParam(2, _templateId, rows);
            dbBind.BindParam(3, _count, rows);
        }

        /* Params */
        int64 _characterId;
        int32 _slotId[MAX_ROWS];
        int32 _templateId[MAX_ROWS];
        int32 _count[MAX_ROWS];
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    try
    {
        // 해당 캐릭터의 장비 아이템 정보를 갱신한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            DECLARE @character_id BIGINT = (?); 

            -- 1. 임시 테이블 생성
            SELECT *
            INTO #TempTable
            FROM [dbo].[CharactersMiscItems]
            WHERE 1 = 0;

            -- 2. 임시 테이블에 INSERT
            INSERT INTO #TempTable (character_id, slot_id, template_id, count)
            VALUES (@character_id, ?, ?, ?)

            -- 3. MERGE 진행
            MERGE INTO [dbo].[CharactersMiscItems] AS T
            USING #TempTable AS S
            ON (T.character_id = S.character_id AND T.slot_id = S.slot_id)

            -- 3-1). 둘 다 행이 존재 -> UPDATE
            WHEN MATCHED THEN
                UPDATE SET template_id = S.template_id, count = S.count

            -- 3-2). DB 테이블에 행이 없음 -> INSERT
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (character_id, slot_id, template_id, count)
                VALUES (S.character_id, S.slot_id, S.template_id, S.count)

            -- 3-3). 임시 테이블에 행이 없음 -> DELETE
            WHEN NOT MATCHED BY SOURCE AND T.character_id = @character_id THEN
                DELETE;

            -- 4. 임시 테이블 삭제
            DROP TABLE #TempItems;
        )SQL");

        PlayerRef player = static_pointer_cast<GameSession>(session)->player;
        Protocol::ObjectInfo* objectInfo = static_pointer_cast<Object>(player)->objectInfo;

        int32 rows = 0;
        int32 processedRows = 0;
        BindObject bindObject(dbBind, objectInfo, player, OUT rows);

        dbConn->SetParamSetSize(rows);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        if (rows != processedRows)
            throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);

        GDBConnectionPool->Push(dbConn);
        return false;
    }

    GDBConnectionPool->Push(dbConn);
    return true;
}
