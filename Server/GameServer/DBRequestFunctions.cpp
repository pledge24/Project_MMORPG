#include "pch.h"
#include "DBRequestFunctions.h"
#include "EncodingConverter.h"
#include "Player.h"
#include "Global.h"

/*-------------------------
    DBRequestFunctions
--------------------------*/

void DBRequestFunctions::GetUserCharactersData(SessionRef session, int64 userId)
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
            throw wstring(L"서버 내부 오류");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_LOGIN pkt;

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
        SEND_PACKET(pkt);
    }
    catch (wstring& cause)
    {
        wcerr << cause << endl;

        Protocol::S_LOGIN pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(GetCharacterData): " << err.what() << endl;

        Protocol::S_LOGIN pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }
    
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
            throw wstring(L"서버 내부 오류");

        if (bindObject._characterId == -1)
            throw wstring(L"이미 존재하는 캐릭터 이름입니다.");

        dbConn->Fetch();

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(true);
        pkt.set_character_id(bindObject._characterId);
        SEND_PACKET(pkt);
    }
    catch (wstring& cause)
    {
        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(false);
        pkt.set_cause(EncodingConverter::WCharToString(cause.c_str()));
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        Protocol::S_CREATE_CHARACTER pkt;
        pkt.set_success(false);
        pkt.set_cause("알 수 없는 오류");
        SEND_PACKET(pkt);
    }

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
            throw wstring(L"서버 내부 오류");

        if (dbConn->GetRowCount() <= 0)
            throw wstring(L"캐릭터 삭제에 실패했습니다");

        // 패킷으로 만들어서 클라이언트에게 보낸다.
        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(true);
        pkt.set_character_id(characterId);
        SEND_PACKET(pkt);
    }
    catch (wstring& cause)
    {
        wcerr << cause << endl;

        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }
    catch (exception& err)
    {
        cerr << "Unexpected Error(CreateCharacter): " << err.what() << endl;

        Protocol::S_DELETE_CHARACTER pkt;
        pkt.set_success(false);
        SEND_PACKET(pkt);
    }

    GDBConnectionPool->Push(dbConn);
}

void DBRequestFunctions::GetEnterGameData(SessionRef session, int64 characterId)
{
    PlayerRef player = static_pointer_cast<GameSession>(session)->player;

    // 1. 캐릭터 기본 정보 다시 가져오기(이름, 레벨 등 필요)
    if (GetCharacterData(session, characterId) == false)
    {
        cout << "Error In GetCharacterData" << endl;
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState)가져오기
    if (GetCharacterLastStateData(session, characterId) == false)
    {
        cout << "Error In GetCharacterLastStateData" << endl;
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 가져오기
    if (GetAllCharacterItems(session, characterId) == false)
    {
        cout << "Error In GetCharacterInventoryData" << endl;
        return;
    }

    // DB에서 가져온 스펙을 기반으로 최종 스텟 계산
    bool success = player->Init();
    
    // 패킷으로 만들어서 클라이언트에게 보낸다.
    Protocol::S_ENTER_GAME pkt;
    pkt.set_success(true);
    pkt.mutable_player()->CopyFrom(*player->objectInfo);
    SEND_PACKET(pkt);
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

    DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
        EXEC GetMaxItemUID;
    )SQL");

    BindObject bindObject(dbBind);

    if (dbBind.Execute() == false)
        return false;

    if (dbConn->Fetch() == false)
        return false;

    GNextItemUID = bindObject._maxItemUID + 1; // itemUid저장

    GDBConnectionPool->Push(dbConn);

    return true;
}

bool DBRequestFunctions::GetCharacterData(SessionRef session, int64 characterId)
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

    // 해당 유저의 캐릭터 기본 정보들을 가져온다.
    DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
        SELECT class_id, character_name, level
        FROM [dbo].[Characters]
        WHERE character_id = (?)
        ORDER BY created_at
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        return false;

    PlayerRef player = static_pointer_cast<GameSession>(session)->player;

    dbConn->Fetch();
    
    Protocol::PlayerInfo* playerInfo = player->playerInfo;

    playerInfo->set_character_id(bindObject._characterId);
    playerInfo->set_class_((Protocol::CharacterClass)bindObject._classId);
    playerInfo->set_name(EncodingConverter::WCharToString(bindObject._characterName));
    playerInfo->set_level(bindObject._level);
    
    GDBConnectionPool->Push(dbConn);

    return true;
}

bool DBRequestFunctions::GetCharacterLastStateData(SessionRef session, int64 characterId)
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

    GDBConnectionPool->Push(dbConn);

    return true;
}

bool DBRequestFunctions::GetAllCharacterItems(SessionRef session, int64 characterId)
{
    // 1. 캐릭터 장비 아이템 가져오기
    if (GetCharactersGearItems(session, characterId) == false)
    {
        cout << "Error In GetCharactersGearItems" << endl;
        return false;
    }

    // 2. 캐릭터 소비 아이템 가져오기
    if (GetCharactersConsumableItems(session, characterId) == false)
    {
        cout << "Error In GetCharactersConsumableItems" << endl;
        return false;
    }

    // 3. 캐릭터 기타 아이템 가져오기
    if (GetCharactersMiscItems(session, characterId) == false)
    {
        cout << "Error In GetCharactersMiscItems" << endl;
        return false;
    }

    return true;
}

bool DBRequestFunctions::GetCharactersGearItems(SessionRef session, int64 characterId)
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

    // 해당 캐릭터의 장비 아이템 정보를 가져온다.
    DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
        SELECT item_uid, template_id, slot_id, enhance, durability, additional_physical_attack, additional_magical_attack
        FROM [dbo].[CharactersGearItems]
        WHERE character_id = (?)
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        return false;

    PlayerRef player = static_pointer_cast<GameSession>(session)->player;
    Protocol::PlayerInfo* playerInfo = player->playerInfo;
    Protocol::Inventory* inventory = playerInfo->mutable_inventory();

    while (dbConn->Fetch())
    {
        // Player->playerInfo
        Protocol::Slot* slot = bindObject._isEquipped ? playerInfo->add_equipped_gear() : inventory->add_gear();
        Protocol::Item* item = slot->mutable_item();
        Protocol::GearInfo* gearInfo = item->mutable_gearinfo();

        slot->set_slot_id(bindObject._slotId);
        slot->set_type(bindObject._isEquipped ? Protocol::SlotType::SLOT_TYPE_EQUIPPED : Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR);
        slot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
        slot->set_count(1);

        item->set_template_id(bindObject._templateId);
        item->set_item_uid(bindObject._itemUid);

        gearInfo->set_enhance_level(bindObject._enhance);
        gearInfo->set_durability(bindObject._durability);
        gearInfo->set_additional_physical_attack(bindObject._additionalMagicalAttack);
        gearInfo->set_additional_magical_attack(bindObject._additionalMagicalAttack);
    }

    GDBConnectionPool->Push(dbConn);

    return true;
}

bool DBRequestFunctions::GetCharactersConsumableItems(SessionRef session, int64 characterId)
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
            dbBind.BindCol(0, _templateId);
            dbBind.BindCol(1, _slotId);
            dbBind.BindCol(2, _count);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int32 _templateId;
        int32 _slotId;
        int32 _count;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    // 해당 캐릭터의 소비 아이템 정보를 가져온다.
    DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
        SELECT template_id, slot_id, count
        FROM [dbo].[CharactersConsumableItems]
        WHERE character_id = (?)
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        return false;

    PlayerRef player = static_pointer_cast<GameSession>(session)->player;
    Protocol::PlayerInfo* playerInfo = player->playerInfo;
    Protocol::Inventory* inventory = playerInfo->mutable_inventory();

    while (dbConn->Fetch())
    {
        Protocol::Slot* slot = inventory->add_consumables();
        Protocol::Item* item = slot->mutable_item();

        slot->set_slot_id(bindObject._slotId);
        slot->set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE);
        slot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
        slot->set_count(bindObject._count);

        item->set_template_id(bindObject._templateId);
    }

    GDBConnectionPool->Push(dbConn);

    return true;
}

bool DBRequestFunctions::GetCharactersMiscItems(SessionRef session, int64 characterId)
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
            dbBind.BindCol(0, _templateId);
            dbBind.BindCol(1, _slotId);
            dbBind.BindCol(2, _count);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int32 _templateId;
        int32 _slotId;
        int32 _count;
    };

    DBConnection* dbConn = GDBConnectionPool->Pop();

    // 해당 캐릭터의 기타 아이템 정보를 가져온다.
    DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
        SELECT template_id, slot_id, count
        FROM [dbo].[CharactersMiscItems]
        WHERE character_id = (?)
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        return false;

    PlayerRef player = static_pointer_cast<GameSession>(session)->player;
    Protocol::PlayerInfo* playerInfo = player->playerInfo;
    Protocol::Inventory* inventory = playerInfo->mutable_inventory();

    while (dbConn->Fetch())
    {
        Protocol::Slot* slot = inventory->add_miscellaneous();
        Protocol::Item* item = slot->mutable_item();

        slot->set_slot_id(bindObject._slotId);
        slot->set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC);
        slot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
        slot->set_count(bindObject._count);

        item->set_template_id(bindObject._templateId);
    }

    GDBConnectionPool->Push(dbConn);

    return true;
}
