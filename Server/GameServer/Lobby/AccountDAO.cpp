#include "Core/pch.h"
#include "Lobby/AccountDAO.h"
#include "DB/DAOCommon.h"

int32 AccountDAO::RecordLogin(DBConnection& conn, int64 accountId)
{
    const int PARAMS = 2;
    const int COLS = 1;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, int64 accountId)
            : _accountId(accountId), _defaultSlotCount(Gamedata::GetCharacterCreation().defaultCharacterSlotCount)
        {
            BindParam(dbBind);
            BindCol(dbBind);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindParam(0, _accountId);
            dbBind.BindParam(1, _defaultSlotCount);
        }

        void BindCol(DBBind<PARAMS, COLS>& dbBind)
        {
            dbBind.BindCol(0, _slotCount);
        }

        //~ Params
        int64 _accountId;
        int32 _defaultSlotCount;

        //~ Cols
        int32 _slotCount = 0;
    };


    // 로그인은 아무 DB 큐에서나 돌아서 같은 계정의 로그인이 동시에 올 수 있다.
    // 갱신이 범위 잠금(SERIALIZABLE)을 잡아, 행이 없을 때 두 로그인이 함께 INSERT해 기본 키 위반으로 실패하지 않게 한다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SET NOCOUNT ON;
        SET XACT_ABORT ON;

        DECLARE @account_id BIGINT = (?);
        DECLARE @default_slot_count INT = (?);

        BEGIN TRANSACTION;

        UPDATE dbo.Account WITH (UPDLOCK, SERIALIZABLE)
        SET LastLoginAt = SYSUTCDATETIME()
        WHERE AccountId = @account_id;

        IF @@ROWCOUNT = 0
            INSERT INTO dbo.Account (AccountId, CharacterSlotCount, LastLoginAt)
            VALUES (@account_id, @default_slot_count, SYSUTCDATETIME());

        COMMIT TRANSACTION;

        SELECT CharacterSlotCount
        FROM dbo.Account
        WHERE AccountId = @account_id;
    )SQL");

    BindObject bindObject(dbBind, accountId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.Fetch() == false)
        throw DBError(__func__, "결과 행이 없다");

    return bindObject._slotCount;
}
