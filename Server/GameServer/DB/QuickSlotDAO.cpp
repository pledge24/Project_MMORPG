#include "Core/pch.h"
#include "DB/QuickSlotDAO.h"
#include "DB/DAOCommon.h"
#include "Game/Entities/PlayerSaveData.h"

namespace
{
    // 저장할 퀵슬롯 행을 배열 파라미터로 묶는다. 바인딩 번호는 INSERT_QUICK_SLOT_SOURCE의 물음표 순서와 맞아야 한다.
    struct QuickSlotRowsBinding
    {
        static constexpr int32 PARAMS = 3;

        QuickSlotRowsBinding(DBBind<PARAMS, 0>& dbBind, const vector<QuickSlot>& quickSlots)
        {
            const int32 rowCount = static_cast<int32>(quickSlots.size());
            for (int32 i = 0; i < rowCount; i++)
            {
                const QuickSlot& quickSlot = quickSlots[i];
                _slotIndex[i] = quickSlot.slotIndex;
                _refType[i] = static_cast<int32>(quickSlot.ref->type);
                _refId[i] = quickSlot.ref->id;
            }

            dbBind.BindParamSet(0, _slotIndex, rowCount);
            dbBind.BindParamSet(1, _refType, rowCount);
            dbBind.BindParamSet(2, _refId, rowCount);
        }

        int32 _slotIndex[MAX_PARAM_ROWS] = {};
        int32 _refType[MAX_PARAM_ROWS] = {};
        int32 _refId[MAX_PARAM_ROWS] = {};
    };

    constexpr const WCHAR* INSERT_QUICK_SLOT_SOURCE = LR"SQL(
        INSERT INTO #QuickSlotSource (SlotIndex, RefType, RefId)
        VALUES (?, ?, ?)
    )SQL";

    // 저장할 행을 담을 임시 테이블을 새로 만든다. 이 연결의 앞선 저장이 실패해 테이블이 남아 있으면 지우고 만든다.
    // 파라미터 없이 실행해야 한다. 파라미터가 있는 실행은 드라이버가 sp_executesql로 보내고, 그 안에서 만든 임시 테이블은 실행이 끝나면 사라진다.
    void CreateQuickSlotSource(DBConnection& conn)
    {
        DBBind<0, 0> dbBind(conn, LR"SQL(
            IF OBJECT_ID(N'tempdb..#QuickSlotSource') IS NOT NULL
                DROP TABLE #QuickSlotSource;

            CREATE TABLE #QuickSlotSource
            (
                SlotIndex TINYINT NOT NULL PRIMARY KEY,
                RefType   TINYINT NOT NULL,
                RefId     INT     NOT NULL
            );
        )SQL");

        if (dbBind.Execute() == false)
            throw DBError(__func__, "쿼리 실행 실패");
    }

    // 임시 테이블의 행으로 그 캐릭터의 QuickSlot을 맞추고 임시 테이블을 지운다.
    void MergeQuickSlotSource(DBConnection& conn, int64 characterId)
    {
        const int PARAMS = 1;
        const int COLS = 0;

        struct BindObject
        {
            BindObject(DBBind<PARAMS, COLS>& dbBind, int64 characterId) : _characterId(characterId)
            {
                BindParam(dbBind);
            }

            void BindParam(DBBind<PARAMS, COLS>& dbBind)
            {
                dbBind.BindParam(0, _characterId);
            }

            //~ Params
            int64 _characterId;
        };


        // 대상을 그 캐릭터의 행으로 좁혀야 사본에 없는 행을 지울 때 다른 캐릭터의 행을 지우지 않는다.
        // 빈 퀵슬롯은 사본에 없으므로 그 행이 지워진다.
        // XACT_ABORT가 켜져 있어 어느 문장이 실패해도 실행이 그 자리에서 실패로 끝난다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            SET NOCOUNT ON;
            SET XACT_ABORT ON;

            DECLARE @character_id BIGINT = (?);

            WITH OwnedQuickSlot AS
            (
                SELECT CharacterId, SlotIndex, RefType, RefId
                FROM dbo.QuickSlot
                WHERE CharacterId = @character_id
            )
            MERGE OwnedQuickSlot AS T
            USING #QuickSlotSource AS S
                ON T.SlotIndex = S.SlotIndex
            WHEN MATCHED THEN
                UPDATE SET
                    RefType = S.RefType,
                    RefId = S.RefId
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (CharacterId, SlotIndex, RefType, RefId)
                VALUES (@character_id, S.SlotIndex, S.RefType, S.RefId)
            WHEN NOT MATCHED BY SOURCE THEN
                DELETE;

            DROP TABLE #QuickSlotSource;
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBError(__func__, "쿼리 실행 실패");
    }
}

void QuickSlotDAO::LoadQuickSlots(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress)
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
            dbBind.BindCol(0, _slotIndex);
            dbBind.BindCol(1, _refType);
            dbBind.BindCol(2, _refId);
        }

        //~ Params
        int64 _characterId;

        //~ Cols
        int32 _slotIndex = 0;
        int32 _refType = 0;
        int32 _refId = 0;
    };


    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SELECT SlotIndex, RefType, RefId
        FROM dbo.QuickSlot
        WHERE CharacterId = (?)
        ORDER BY SlotIndex
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    while (conn.Fetch())
    {
        const QuickSlotRef ref{ static_cast<QuickSlotRefType>(bindObject._refType), bindObject._refId };
        progress.quickSlots.push_back(QuickSlot{ bindObject._slotIndex, ref });
    }
}

void QuickSlotDAO::SaveQuickSlots(DBConnection& conn, const PlayerSaveData& data)
{
    const int64 characterId = data.progress.characterId;
    for (const QuickSlot& quickSlot : data.progress.quickSlots)
    {
        // 빈 퀵슬롯을 행으로 넣으면 RefType 0이 CHECK 제약에 걸려 저장 전체가 되돌려진다.
        if (quickSlot.ref.has_value() == false || quickSlot.ref->type == QuickSlotRefType::NONE)
            throw DBError(__func__, format("빈 퀵슬롯 {}을 행으로 저장하려 했다", quickSlot.slotIndex));
    }

    CreateQuickSlotSource(conn);

    // 퀵슬롯 개수의 상한(256)이 배열 파라미터 한 번에 묶을 수 있는 행 수보다 커서 나눠 넣는다.
    // MERGE는 모든 행을 넣은 뒤 한 번만 실행해야 사본에 없는 행을 가려낼 수 있다.
    const vector<QuickSlot>& rows = data.progress.quickSlots;
    for (size_t begin = 0; begin < rows.size(); begin += MAX_PARAM_ROWS)
    {
        const size_t end = (std::min)(begin + MAX_PARAM_ROWS, rows.size());
        const vector<QuickSlot> chunk(rows.begin() + begin, rows.begin() + end);
        ExecuteParamSet<QuickSlotRowsBinding>(conn, __func__, INSERT_QUICK_SLOT_SOURCE, chunk);
    }

    MergeQuickSlotSource(conn, characterId);
}
