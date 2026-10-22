#include "Core/pch.h"
#include "DB/CharacterStateDAO.h"
#include "DB/DAOCommon.h"
#include "Utils/EncodingConverter.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Entities/PlayerSaveData.h"

CharacterLoadResult CharacterStateDAO::LoadCharacter(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress)
{
    const int PARAMS = 3;
    const int COLS = 13;

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
            dbBind.BindCol(3, _hasProgress);
            dbBind.BindCol(4, _exp);
            dbBind.BindCol(5, _mapId);
            dbBind.BindCol(6, _posX);
            dbBind.BindCol(7, _posY);
            dbBind.BindCol(8, _posZ);
            dbBind.BindCol(9, _direction);
            dbBind.BindCol(10, _currentHp);
            dbBind.BindCol(11, _currentMp);
            dbBind.BindCol(12, _gold);
        }

        //~ Params
        int64 _characterId;
        int64 _userId;
        int32 _goldCurrencyId = Gamedata::GetGoldCurrencyId();

        //~ Cols
        int32 _classId = 0;
        WCHAR _name[CHARACTER_NAME_COLUMN_LENGTH + 1] = {};
        int32 _level = 0;
        int32 _hasProgress = 0;
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
    // 진행 행은 첫 게임 입장이 만들므로 외부 조인하고, 행이 있는지를 따로 받는다.
    // 바인딩한 변수에는 NULL이 쓰이지 않으므로 진행 행이 없을 때의 열은 0으로 바꿔 읽는다.
    // 골드 행이 없으면 -1을 돌려받아 DB가 어긋난 것으로 판정한다.
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

        SELECT c.ClassId, c.Name, c.Level,
               CASE WHEN s.CharacterId IS NULL THEN 0 ELSE 1 END,
               ISNULL(s.Exp, 0), ISNULL(s.MapId, 0), ISNULL(s.PosX, 0), ISNULL(s.PosY, 0), ISNULL(s.PosZ, 0),
               ISNULL(s.Direction, 0), ISNULL(s.CurrentHp, 0), ISNULL(s.CurrentMp, 0),
               ISNULL(g.Amount, -1)
        FROM dbo.[Character] AS c
        LEFT JOIN dbo.CharacterState AS s
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
        return CharacterLoadResult::NOT_FOUND;

    progress.characterId = characterId;
    progress.classId = bindObject._classId;
    progress.name = EncodingConverter::WCharToString(bindObject._name);
    progress.level = bindObject._level;

    if (bindObject._hasProgress == 0)
        return CharacterLoadResult::NO_PROGRESS;

    if (bindObject._gold < 0)
        throw DBError(__func__, format("캐릭터 {}에 골드 재화(번호 {}) 행이 없다", characterId, bindObject._goldCurrencyId));

    progress.mapId = bindObject._mapId;
    progress.pos = vector3D(bindObject._posX, bindObject._posY, bindObject._posZ);
    progress.yaw = bindObject._direction;
    progress.totalExp = bindObject._exp;
    progress.hp = bindObject._currentHp;
    progress.mp = bindObject._currentMp;
    progress.gold = bindObject._gold;

    return CharacterLoadResult::LOADED;
}

void CharacterStateDAO::CreateInitialProgress(DBConnection& conn, const PlayerProgress& progress)
{
    // 상태 행.
    {
        const int PARAMS = 9;
        const int COLS = 0;

        struct BindObject
        {
            BindObject(DBBind<PARAMS, COLS>& dbBind, const PlayerProgress& progress)
                : _characterId(progress.characterId), _exp(progress.totalExp), _mapId(progress.mapId),
                  _posX(progress.pos.x), _posY(progress.pos.y), _posZ(progress.pos.z), _direction(progress.yaw),
                  _currentHp(progress.hp), _currentMp(progress.mp)
            {
                BindParam(dbBind);
            }

            void BindParam(DBBind<PARAMS, COLS>& dbBind)
            {
                dbBind.BindParam(0, _characterId);
                dbBind.BindParam(1, _exp);
                dbBind.BindParam(2, _mapId);
                dbBind.BindParam(3, _posX);
                dbBind.BindParam(4, _posY);
                dbBind.BindParam(5, _posZ);
                dbBind.BindParam(6, _direction);
                dbBind.BindParam(7, _currentHp);
                dbBind.BindParam(8, _currentMp);
            }

            //~ Params
            int64 _characterId;
            int64 _exp;
            int32 _mapId;
            float _posX;
            float _posY;
            float _posZ;
            float _direction;
            int32 _currentHp;
            int32 _currentMp;
        };


        // 이미 진행 행이 있으면 기본 키 위반으로 실패한다. 덮어쓰면 그 캐릭터의 진행이 처음으로 돌아간다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            INSERT INTO dbo.CharacterState (CharacterId, Exp, MapId, PosX, PosY, PosZ, Direction, CurrentHp, CurrentMp)
            VALUES ((?), (?), (?), (?), (?), (?), (?), (?), (?))
        )SQL");

        BindObject bindObject(dbBind, progress);

        if (dbBind.Execute() == false)
            throw DBError(__func__, "상태 행 쿼리 실행 실패");

        if (conn.GetRowCount() != 1)
            throw DBError(__func__, "상태 행을 넣지 못했다");
    }

    // 골드 행.
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
                dbBind.BindParam(0, _characterId);
                dbBind.BindParam(1, _goldCurrencyId);
                dbBind.BindParam(2, _gold);
            }

            //~ Params
            int64 _characterId;
            int64 _gold;
            int32 _goldCurrencyId = Gamedata::GetGoldCurrencyId();
        };


        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            INSERT INTO dbo.CharacterCurrency (CharacterId, CurrencyId, Amount)
            VALUES ((?), (?), (?))
        )SQL");

        BindObject bindObject(dbBind, progress.characterId, progress.gold);

        if (dbBind.Execute() == false)
            throw DBError(__func__, "골드 행 쿼리 실행 실패");

        if (conn.GetRowCount() != 1)
            throw DBError(__func__, "골드 행을 넣지 못했다");
    }
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

    BindObject bindObject(dbBind, data.progress.characterId, data.progress.level);

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
        BindObject(DBBind<PARAMS, COLS>& dbBind, const PlayerProgress& progress)
            : _exp(progress.totalExp), _mapId(progress.mapId), _posX(progress.pos.x), _posY(progress.pos.y), _posZ(progress.pos.z),
              _direction(progress.yaw), _currentHp(progress.hp), _currentMp(progress.mp), _characterId(progress.characterId)
        {
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

    BindObject bindObject(dbBind, data.progress);

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

    BindObject bindObject(dbBind, data.progress.characterId, data.progress.gold);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.GetRowCount() != 1)
        throw DBError(__func__, "바뀐 행 수가 맞지 않는다");
}
