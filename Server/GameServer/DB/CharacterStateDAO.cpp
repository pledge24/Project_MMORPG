#include "Core/pch.h"
#include "DB/CharacterStateDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Entities/PlayerSaveData.h"

bool CharacterStateDAO::LoadCharacter(int64 userId, int64 characterId, OUT PlayerProgress& progress)
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

        //~ Params
        int64 _characterId;
        int64 _userId;

        //~ Cols
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

        BindObject bindObject(dbBind, characterId, userId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        // 행이 없으면 바인딩 버퍼는 초기화되지 않은 값이다. 채우지 않고 실패로 끝낸다.
        if (dbConn->Fetch() == false)
            throw DBCustomError::SQL_FETCH_FAIL;

        Protocol::PlayerInfo* playerInfo = &progress.playerInfo;

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

bool CharacterStateDAO::LoadLastState(int64 characterId, OUT PlayerProgress& progress)
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

        //~ Params
        int64 _characterId;

        //~ Cols
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

        auto* statMappings = progress.statInfo.mutable_info();
        (*statMappings)[Protocol::STAT_TYPE_EXP] = bindObject._exp;
        (*statMappings)[Protocol::STAT_TYPE_HP] = bindObject._curHp;
        (*statMappings)[Protocol::STAT_TYPE_MP] = bindObject._curMp;
        (*statMappings)[Protocol::STAT_TYPE_PHYSICAL_ATTACK] = bindObject._curPhysicalAttack;
        (*statMappings)[Protocol::STAT_TYPE_MAGICAL_ATTACK] = bindObject._curMagicalAttack;

        progress.playerInfo.set_room_id(bindObject._roomId);
        progress.playerInfo.set_map_id(bindObject._mapId);

        Protocol::Vector* pos = progress.posInfo.mutable_pos();
        pos->set_x(bindObject._posX);
        pos->set_y(bindObject._posY);
        pos->set_z(bindObject._posZ);
        progress.posInfo.set_yaw(bindObject._rotYaw);

        progress.possession.set_gold(bindObject._gold);

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

        //~ Params
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

        BindObject bindObject(dbBind, data.progress.playerInfo.character_id(), data.progress.playerInfo.level());

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
            const Protocol::PlayerInfo& playerInfo = data.progress.playerInfo;
            const Protocol::StatInfo& statInfo = data.progress.statInfo;
            const Protocol::PosInfo& posInfo = data.progress.posInfo;
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
            _gold = data.progress.possession.gold();
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

        //~ Params
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
