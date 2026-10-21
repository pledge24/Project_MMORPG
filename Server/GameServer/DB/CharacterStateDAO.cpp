#include "Core/pch.h"
#include "DB/CharacterStateDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Game/Characters/CharacterCreation.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Entities/PlayerSaveData.h"

bool CharacterStateDAO::LoadCharacter(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress)
{
    const int PARAMS = 3;
    const int COLS = 12;

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
            dbBind.BindParam(2, _goldCurrencyId);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _classId);
            dbBind.BindCol(1, _name);
            dbBind.BindCol(2, _level);
            dbBind.BindCol(3, _exp);
            dbBind.BindCol(4, _mapId);
            dbBind.BindCol(5, _posX);
            dbBind.BindCol(6, _posY);
            dbBind.BindCol(7, _posZ);
            dbBind.BindCol(8, _direction);
            dbBind.BindCol(9, _currentHp);
            dbBind.BindCol(10, _currentMp);
            dbBind.BindCol(11, _gold);
        }

        //~ Params
        int64 _characterId;
        int64 _userId;
        int32 _goldCurrencyId = Gamedata::GetGoldCurrencyId();

        //~ Cols
        int32 _classId = 0;
        WCHAR _name[CharacterCreation::MAX_NAME_LENGTH + 1] = {};
        int32 _level = 0;
        int64 _exp = 0;
        int32 _mapId = 0;
        float _posX = 0.f;
        float _posY = 0.f;
        float _posZ = 0.f;
        float _direction = 0.f;
        int32 _currentHp = 0;
        int32 _currentMp = 0;
        int64 _gold = 0;
    };


    // AccountId를 함께 대조한다. CharacterId는 클라이언트 값이라, 빼면 다른 계정의 캐릭터로 입장할 수 있다.
    // 상태 행은 캐릭터를 만들 때 한 트랜잭션에서 함께 넣으므로 내부 조인한다.
    // 골드 행이 없으면 -1을 돌려받아 DB가 어긋난 것으로 판정한다. 바인딩한 변수에는 NULL이 쓰이지 않는다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SET NOCOUNT ON;

        DECLARE @character_id BIGINT = (?);
        DECLARE @account_id BIGINT = (?);
        DECLARE @gold_currency_id INT = (?);

        UPDATE dbo.[Character]
        SET LastLoginAt = SYSUTCDATETIME()
        WHERE CharacterId = @character_id
          AND AccountId = @account_id
          AND DeletedAt IS NULL;

        SELECT c.ClassId, c.Name, c.Level, s.Exp, s.MapId, s.PosX, s.PosY, s.PosZ, s.Direction, s.CurrentHp, s.CurrentMp,
               ISNULL(g.Amount, -1)
        FROM dbo.[Character] AS c
        INNER JOIN dbo.CharacterState AS s
            ON s.CharacterId = c.CharacterId
        LEFT JOIN dbo.CharacterCurrency AS g
            ON g.CharacterId = c.CharacterId
           AND g.CurrencyId = @gold_currency_id
        WHERE c.CharacterId = @character_id
          AND c.AccountId = @account_id
          AND c.DeletedAt IS NULL;
    )SQL");

    BindObject bindObject(dbBind, characterId, userId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    // 행이 없으면 이 계정의 캐릭터가 아니다. 바인딩 버퍼는 읽은 값이 아니므로 채우지 않는다.
    if (conn.Fetch() == false)
        return false;

    if (bindObject._gold < 0)
        throw DBError(__func__, format("캐릭터 {}에 골드 재화(번호 {}) 행이 없다", characterId, bindObject._goldCurrencyId));

    Protocol::PlayerInfo* playerInfo = &progress.playerInfo;
    playerInfo->set_character_id(characterId);
    playerInfo->set_class_((Protocol::CharacterClass)bindObject._classId);
    playerInfo->set_name(EncodingConverter::WCharToString(bindObject._name));
    playerInfo->set_level(bindObject._level);
    playerInfo->set_map_id(bindObject._mapId);

    Protocol::Vector* pos = progress.posInfo.mutable_pos();
    pos->set_x(bindObject._posX);
    pos->set_y(bindObject._posY);
    pos->set_z(bindObject._posZ);
    progress.posInfo.set_yaw(bindObject._direction);

    // 경험치는 누적값이다. 레벨 안의 값과 요구치는 입장할 때 레벨 표로 계산한다.
    auto* statMappings = progress.statInfo.mutable_info();
    (*statMappings)[Protocol::STAT_TYPE_EXP] = bindObject._exp;
    (*statMappings)[Protocol::STAT_TYPE_HP] = bindObject._currentHp;
    (*statMappings)[Protocol::STAT_TYPE_MP] = bindObject._currentMp;

    progress.gold = bindObject._gold;

    return true;
}

void CharacterStateDAO::SaveCharacter(DBConnection& conn, const PlayerSaveData& data)
{
    const int PARAMS = 2;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int32 level) : _characterId(characterId), _level(level)
        {
            BindParam(dbBind);
        }

        // 번호는 아래 SQL의 물음표 순서다. SET이 먼저, WHERE가 나중이다. 어긋나면 다른 캐릭터의 레벨을 덮어쓴다.
        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _level);
            dbBind.BindParam(1, _characterId);
        }

        //~ Params
        int64 _characterId;
        int32 _level;
    };


    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        UPDATE dbo.[Character]
        SET Level = (?)
        WHERE CharacterId = (?)
    )SQL");

    BindObject bindObject(dbBind, data.progress.playerInfo.character_id(), data.progress.playerInfo.level());

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    // 행이 없으면 UPDATE는 성공하고 아무것도 바꾸지 않는다. 저장이 사라진 것을 실패로 알린다.
    if (conn.GetRowCount() != 1)
        throw DBError(__func__, "바뀐 행 수가 맞지 않는다");
}

void CharacterStateDAO::SaveState(DBConnection& conn, const PlayerSaveData& data)
{
    const int PARAMS = 9;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const PlayerSaveData& data)
        {
            const Protocol::PlayerInfo& playerInfo = data.progress.playerInfo;
            const Protocol::PosInfo& posInfo = data.progress.posInfo;
            const auto& statMappings = data.progress.statInfo.info();

            // 저장 사본의 경험치는 누적값이다(Player::MakeSaveData).
            _exp = statMappings.at(Protocol::STAT_TYPE_EXP);
            _mapId = playerInfo.map_id();
            _posX = posInfo.pos().x();
            _posY = posInfo.pos().y();
            _posZ = posInfo.pos().z();
            _direction = posInfo.yaw();
            _currentHp = static_cast<int32>(statMappings.at(Protocol::STAT_TYPE_HP));
            _currentMp = static_cast<int32>(statMappings.at(Protocol::STAT_TYPE_MP));
            _characterId = playerInfo.character_id();

            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _exp);
            dbBind.BindParam(1, _mapId);
            dbBind.BindParam(2, _posX);
            dbBind.BindParam(3, _posY);
            dbBind.BindParam(4, _posZ);
            dbBind.BindParam(5, _direction);
            dbBind.BindParam(6, _currentHp);
            dbBind.BindParam(7, _currentMp);
            dbBind.BindParam(8, _characterId);
        }

        //~ Params
        int64 _exp;
        int32 _mapId;
        float _posX;
        float _posY;
        float _posZ;
        float _direction;
        int32 _currentHp;
        int32 _currentMp;
        int64 _characterId;
    };


    // UpdatedAt의 기본값은 INSERT에만 쓰이므로 갱신할 때 직접 넣는다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        UPDATE dbo.CharacterState
        SET Exp = (?), MapId = (?), PosX = (?), PosY = (?), PosZ = (?), Direction = (?), CurrentHp = (?), CurrentMp = (?),
            UpdatedAt = SYSUTCDATETIME()
        WHERE CharacterId = (?)
    )SQL");

    BindObject bindObject(dbBind, data);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.GetRowCount() != 1)
        throw DBError(__func__, "바뀐 행 수가 맞지 않는다");
}

void CharacterStateDAO::SaveGold(DBConnection& conn, const PlayerSaveData& data)
{
    const int PARAMS = 3;
    const int COLS = 0;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId, int64 gold) : _characterId(characterId), _gold(gold)
        {
            BindParam(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _gold);
            dbBind.BindParam(1, _characterId);
            dbBind.BindParam(2, _goldCurrencyId);
        }

        //~ Params
        int64 _characterId;
        int64 _gold;
        int32 _goldCurrencyId = Gamedata::GetGoldCurrencyId();
    };


    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        UPDATE dbo.CharacterCurrency
        SET Amount = (?), UpdatedAt = SYSUTCDATETIME()
        WHERE CharacterId = (?)
          AND CurrencyId = (?)
    )SQL");

    BindObject bindObject(dbBind, data.progress.playerInfo.character_id(), data.progress.gold);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.GetRowCount() != 1)
        throw DBError(__func__, "바뀐 행 수가 맞지 않는다");
}
