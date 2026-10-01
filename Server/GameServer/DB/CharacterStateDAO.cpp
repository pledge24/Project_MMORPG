#include "pch.h"
#include "CharacterStateDAO.h"
#include "DAOCommon.h"
#include "EncodingConverter.h"
#include "Player.h"
#include "PlayerSaveData.h"

/*-------------------------
    CharacterStateDAO
--------------------------*/

bool CharacterStateDAO::LoadCharacter(SessionRef session, int64 characterId)
{
    const int PARAMS = 2;
    const int COLS = 3;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int64 userId) : _characterId(characterId), _userId(userId)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _characterId);
            dbBind.BindParam(1, _userId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _classId);
            dbBind.BindCol(1, _characterName);
            dbBind.BindCol(2, _level);
        }

        /* Params */
        int64 _characterId;
        int64 _userId;

        /* Cols */
        int32 _classId;
        WCHAR _characterName[100];
        int16 _level;
    };

    DBConnectionGuard dbConn;

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 가져온다.
        // 이 계정의 캐릭터가 아니면 행이 없다. 클라이언트가 보낸 character_id를 믿지 않는다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT class_id, character_name, level
            FROM [dbo].[Characters]
            WHERE character_id = (?) AND user_id = (?)
        )SQL");

        GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
        BindObject bindObject(dbBind, characterId, gameSession->_userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        // 행이 없으면 바인딩 버퍼는 초기화되지 않은 값이다. 채우지 않고 실패로 끝낸다.
        if (dbConn->Fetch() == false)
            throw DBCustomError::SQL_FETCH_FAIL;

        PlayerRef player = gameSession->_player;

        Protocol::PlayerInfo* playerInfo = player->_playerInfo;

        playerInfo->set_character_id(bindObject._characterId);
        playerInfo->set_class_((Protocol::CharacterClass)bindObject._classId);
        playerInfo->set_name(EncodingConverter::WCharToString(bindObject._characterName));
        playerInfo->set_level(bindObject._level);
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool CharacterStateDAO::LoadLastState(SessionRef session, int64 characterId)
{
    const int PARAMS = 1;
    const int COLS = 12;

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
            dbBind.BindCol(5, _roomId);
            dbBind.BindCol(6, _mapId);
            dbBind.BindCol(7, _posX);
            dbBind.BindCol(8, _posY);
            dbBind.BindCol(9, _posZ);
            dbBind.BindCol(10, _rotYaw);
            dbBind.BindCol(11, _gold);
        }

        /* Params */
        int64 _characterId;

        /* Cols */
        int64 _exp;
        int64 _curHp;
        int64 _curMp;
        int64 _curPhysicalAttack;
        int64 _curMagicalAttack;
        int32 _roomId;
        int32 _mapId;
        float _posX;
        float _posY;
        float _posZ;
        float _rotYaw;
        int64 _gold;
    };

    DBConnectionGuard dbConn;

    try
    {
        // 해당 유저의 마지막 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT exp, cur_hp, cur_mp, cur_physical_attack, cur_magical_attack, room_id, map_id, pos_x, pos_y, pos_z, rot_yaw, gold
            FROM [dbo].[CharactersLastState] 
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            return false;

        if (dbConn->Fetch() == false)
            return false;

        PlayerRef player = static_pointer_cast<GameSession>(session)->_player;
        Protocol::EntityInfo* entityInfo = player->_entityInfo;
        Protocol::PlayerInfo* playerInfo = player->_playerInfo;
        Protocol::StatInfo* statInfo = player->_statInfo;
        auto* statMappings = statInfo->mutable_info();

        // ==성장 및 스텟 관련==
        const DataTable* classLevelTable = Gamedata::FindClassLevelTable(playerInfo->class_());
        if (classLevelTable == nullptr)
        {
            wcout << L"캐릭터 " << characterId << L"의 클래스 " << playerInfo->class_() << L"에 레벨 표가 없습니다" << '\n';
            return false;
        }

        const DataTable& classLevelDataTable = *classLevelTable;
        auto levelIt = classLevelDataTable.find(playerInfo->level());
        const bool hasExpRequirement = levelIt != classLevelDataTable.end()
            && levelIt->second.contains(JsonProperty::LevelTable::ExpRequirement)
            && levelIt->second.at(JsonProperty::LevelTable::ExpRequirement).is_number();
        if (hasExpRequirement == false)
        {
            wcout << L"캐릭터 " << characterId << L"의 레벨 " << playerInfo->level() << L"이 레벨 표 범위 밖입니다" << '\n';
            return false;
        }

        int64 maxExp = levelIt->second.at(JsonProperty::LevelTable::ExpRequirement);
        statMappings->insert({ (int32)Protocol::STAT_TYPE_EXP, bindObject._exp });
        statMappings->insert({ (int32)Protocol::STAT_TYPE_MAX_EXP, maxExp });

        // 현재 Hp
        statMappings->insert({ (int32)Protocol::STAT_TYPE_HP, bindObject._curHp });
        statMappings->insert({ (int32)Protocol::STAT_TYPE_MP, bindObject._curMp });
        statMappings->insert({ (int32)Protocol::STAT_TYPE_PHYSICAL_ATTACK, bindObject._curPhysicalAttack });
        statMappings->insert({ (int32)Protocol::STAT_TYPE_MAGICAL_ATTACK, bindObject._curMagicalAttack });

        // 지역 설정
        playerInfo->set_room_id(bindObject._roomId);

        // map_id 세팅과 _enteringRoomId 시딩을 함께 처리한다.
        // 클라가 C_ENTER_MAP을 보내지 않으므로 여기서 채우지 않으면
        // 최초 입장(INITIAL) 검증이 _enteringRoomId == -1 로 실패한다.
        player->OnEnterMap(bindObject._mapId, bindObject._roomId);

        // PosInfo 설정
        {
            Protocol::PosInfo spawnPosInfo;
            Protocol::Vector& pos = *spawnPosInfo.mutable_pos();

            spawnPosInfo.set_entity_id(entityInfo->entity_id());
            pos.set_x(bindObject._posX);
            pos.set_y(bindObject._posY);
            pos.set_z(bindObject._posZ);
            spawnPosInfo.set_yaw(bindObject._rotYaw);
            spawnPosInfo.set_state(Protocol::MOVE_STATE_IDLE);

            player->SetPosInfo(spawnPosInfo);
        }

        // ==플레이어 골드 설정==
        player->_possession->set_gold(bindObject._gold);

    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool CharacterStateDAO::SaveCharacter(const PlayerSaveData& data)
{
    const int PARAMS = 2;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int16 level) : _characterId(characterId), _level(level)
        {
            BindParam(dbBind);
        }

        // 번호는 아래 SQL의 물음표 순서다. SET이 먼저, WHERE가 나중이다.
        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _level);
            dbBind.BindParam(1, _characterId);
        }

        /* Params */
        int64 _characterId;
        int16 _level;
    };

    DBConnectionGuard dbConn;

    try
    {
        // 해당 유저의 캐릭터 기본 정보들을 갱신한다(지금은 레벨만 갱신).
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            UPDATE [dbo].[Characters]
            SET level = (?)
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, data.playerInfo.character_id(), data.playerInfo.level());

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool CharacterStateDAO::SaveLastState(const PlayerSaveData& data)
{
    const int PARAMS = 13;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const PlayerSaveData& data)
        {
            const Protocol::PlayerInfo& playerInfo = data.playerInfo;
            const Protocol::StatInfo& statInfo = data.statInfo;
            const Protocol::PosInfo& posInfo = data.posInfo;
            auto& statMappings = statInfo.info();

            _exp = statMappings.at(Protocol::STAT_TYPE_EXP);
            _curHp = statMappings.at(Protocol::STAT_TYPE_HP);
            _curMp = statMappings.at(Protocol::STAT_TYPE_MP);
            _curPhysicalAttack = statMappings.at(Protocol::STAT_TYPE_PHYSICAL_ATTACK);
            _curMagicalAttack = statMappings.at(Protocol::STAT_TYPE_MAGICAL_ATTACK);
            _roomId = playerInfo.room_id();
            _mapId = playerInfo.map_id();
            _posX = posInfo.pos().x();
            _posY = posInfo.pos().y();
            _posZ = posInfo.pos().z();
            _rotYaw = posInfo.yaw();
            _gold = data.possession.gold();
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
            dbBind.BindParam(5, _roomId);
            dbBind.BindParam(6, _mapId);
            dbBind.BindParam(7, _posX);
            dbBind.BindParam(8, _posY);
            dbBind.BindParam(9, _posZ);
            dbBind.BindParam(10, _rotYaw);
            dbBind.BindParam(11, _gold);
            dbBind.BindParam(12, _characterId);
        }

        /* Params */
        int64 _exp;
        int64 _curHp;
        int64 _curMp;
        int64 _curPhysicalAttack;
        int64 _curMagicalAttack;
        int32 _roomId;
        int32 _mapId;
        float _posX;
        float _posY;
        float _posZ;
        float _rotYaw;
        int64 _gold;
        int64 _characterId;
    };

    DBConnectionGuard dbConn;

    try
    {
        // 해당 유저의 마지막 정보를 DB에 갱신한다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            UPDATE [dbo].[CharactersLastState]
            SET exp = (?), cur_hp = (?), cur_mp = (?), cur_physical_attack = (?), cur_magical_attack = (?), room_id = (?), map_id = (?), pos_x = (?), pos_y = (?), pos_z = (?), rot_yaw = (?), gold = (?)
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, data);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        //if (dbConn->GetRowCount() != 1)
        //    throw DBCustomError::SQL_MISMATCHED_GET_ROW_COUNT;
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}
