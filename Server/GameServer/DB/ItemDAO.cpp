#include "Core/pch.h"
#include "DB/ItemDAO.h"
#include "DB/DAOCommon.h"
#include "DB/ItemRows.h"
#include "Game/Entities/PlayerSaveData.h"

namespace
{
    // 저장할 아이템 행을 배열 파라미터로 묶는다. 바인딩 번호는 INSERT_ITEM_SOURCE의 물음표 순서와 맞아야 한다. 어긋나면 값이 다른 열에 저장된다.
    struct ItemRowsBinding
    {
        static constexpr int32 PARAMS = 8;

        ItemRowsBinding(DBBind<PARAMS, 0>& dbBind, const vector<ItemRow>& rows)
        {
            const int32 rowCount = static_cast<int32>(rows.size());
            for (int32 i = 0; i < rowCount; i++)
            {
                const ItemRow& row = rows[i];
                _itemUid[i] = row.itemUid;
                _templateId[i] = row.templateId;
                _itemType[i] = static_cast<int32>(row.itemType);
                _container[i] = static_cast<int32>(row.container);
                _slotIndex[i] = row.slotIndex;
                _stackCount[i] = row.stackCount;
                _enhanceLevel[i] = row.enhanceLevel;
                _durability[i] = row.durability;
            }

            dbBind.BindParamSet(0, _itemUid, rowCount);
            dbBind.BindParamSet(1, _templateId, rowCount);
            dbBind.BindParamSet(2, _itemType, rowCount);
            dbBind.BindParamSet(3, _container, rowCount);
            dbBind.BindParamSet(4, _slotIndex, rowCount);
            dbBind.BindParamSet(5, _stackCount, rowCount);
            dbBind.BindParamSet(6, _enhanceLevel, rowCount);
            dbBind.BindParamSet(7, _durability, rowCount);
        }

        int64 _itemUid[MAX_PARAM_ROWS] = {};
        int32 _templateId[MAX_PARAM_ROWS] = {};
        int32 _itemType[MAX_PARAM_ROWS] = {};
        int32 _container[MAX_PARAM_ROWS] = {};
        int32 _slotIndex[MAX_PARAM_ROWS] = {};
        int32 _stackCount[MAX_PARAM_ROWS] = {};
        int32 _enhanceLevel[MAX_PARAM_ROWS] = {};
        int32 _durability[MAX_PARAM_ROWS] = {};
    };

    constexpr const WCHAR* INSERT_ITEM_SOURCE = LR"SQL(
        INSERT INTO #ItemSource (ItemUid, TemplateId, ItemType, Container, SlotIndex, StackCount, EnhanceLevel, Durability)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )SQL";

    // 저장할 행을 담을 임시 테이블을 새로 만든다. 이 연결의 앞선 저장이 실패해 테이블이 남아 있으면 지우고 만든다.
    // 파라미터 없이 실행해야 한다. 파라미터가 있는 실행은 드라이버가 sp_executesql로 보내고, 그 안에서 만든 임시 테이블은 실행이 끝나면 사라진다.
    // 같은 이유로 여기서 바꾼 SET 설정은 연결에 남는다. SET NOCOUNT ON을 넣으면 이 연결의 뒤 UPDATE가 바꾼 행 수를 받지 못한다.
    void CreateItemSource(DBConnection& conn)
    {
        DBBind<0, 0> dbBind(conn, LR"SQL(
            IF OBJECT_ID(N'tempdb..#ItemSource') IS NOT NULL
                DROP TABLE #ItemSource;

            CREATE TABLE #ItemSource
            (
                ItemUid      BIGINT   NOT NULL PRIMARY KEY,
                TemplateId   INT      NOT NULL,
                ItemType     TINYINT  NOT NULL,
                Container    TINYINT  NOT NULL,
                SlotIndex    SMALLINT NOT NULL,
                StackCount   INT      NOT NULL,
                EnhanceLevel TINYINT  NOT NULL,
                Durability   INT      NOT NULL
            );
        )SQL");

        if (dbBind.Execute() == false)
            throw DBError(__func__, "쿼리 실행 실패");
    }

    // 임시 테이블의 행으로 그 캐릭터의 Item과 ItemGear를 맞추고 임시 테이블을 지운다.
    void MergeItemSource(DBConnection& conn, int64 characterId)
    {
        const int PARAMS = 2;
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
                dbBind.BindParam(1, _gearItemType);
            }

            //~ Params
            int64 _characterId;
            int32 _gearItemType = static_cast<int32>(ItemRowType::GEAR);
        };


        // 대상을 그 캐릭터의 행으로 좁혀야 사본에 없는 행을 지울 때 다른 캐릭터의 행을 지우지 않는다.
        // 다른 캐릭터가 가진 UID가 사본에 있으면 INSERT가 기본 키 위반으로 실패하고 저장 전체가 되돌려진다.
        // 아이템 하나의 템플릿과 종류는 바뀌지 않으므로 자리와 개수만 갱신한다. 종류를 바꾸면 ItemGear의 외래 키가 깨진다.
        // 칸을 맞바꾸는 두 행도 한 문장 안에서 갱신되므로, 고유 제약은 문장이 끝난 상태로 검사된다.
        // 지운 장비의 ItemGear 행은 외래 키의 ON DELETE CASCADE가 함께 지운다.
        // XACT_ABORT가 켜져 있어 어느 문장이 실패해도 실행이 그 자리에서 실패로 끝난다. 꺼져 있으면 실패한 문장 뒤의 문장이 이어서 돈다.
        DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
            SET NOCOUNT ON;
            SET XACT_ABORT ON;

            DECLARE @owner_id BIGINT = (?);
            DECLARE @gear_item_type TINYINT = (?);

            WITH OwnedItem AS
            (
                SELECT ItemUid, OwnerId, TemplateId, ItemType, Container, SlotIndex, StackCount
                FROM dbo.Item
                WHERE OwnerId = @owner_id
            )
            MERGE OwnedItem AS T
            USING #ItemSource AS S
                ON T.ItemUid = S.ItemUid
            WHEN MATCHED THEN
                UPDATE SET
                    Container = S.Container,
                    SlotIndex = S.SlotIndex,
                    StackCount = S.StackCount
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (ItemUid, OwnerId, TemplateId, ItemType, Container, SlotIndex, StackCount)
                VALUES (S.ItemUid, @owner_id, S.TemplateId, S.ItemType, S.Container, S.SlotIndex, S.StackCount)
            WHEN NOT MATCHED BY SOURCE THEN
                DELETE;

            MERGE dbo.ItemGear AS T
            USING
            (
                SELECT ItemUid, ItemType, EnhanceLevel, Durability
                FROM #ItemSource
                WHERE ItemType = @gear_item_type
            ) AS S
                ON T.ItemUid = S.ItemUid
            WHEN MATCHED THEN
                UPDATE SET
                    EnhanceLevel = S.EnhanceLevel,
                    Durability = S.Durability
            WHEN NOT MATCHED BY TARGET THEN
                INSERT (ItemUid, ItemType, EnhanceLevel, Durability)
                VALUES (S.ItemUid, S.ItemType, S.EnhanceLevel, S.Durability);

            DROP TABLE #ItemSource;
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBError(__func__, "쿼리 실행 실패");
    }
}

void ItemDAO::LoadItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress)
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
            dbBind.BindCol(2, _itemType);
            dbBind.BindCol(3, _container);
            dbBind.BindCol(4, _slotIndex);
            dbBind.BindCol(5, _stackCount);
            dbBind.BindCol(6, _enhanceLevel);
            dbBind.BindCol(7, _durability);
        }

        //~ Params
        int64 _characterId;

        //~ Cols
        int64 _itemUid = 0;
        int32 _templateId = 0;
        int32 _itemType = 0;
        int32 _container = 0;
        int32 _slotIndex = 0;
        int32 _stackCount = 0;
        int32 _enhanceLevel = 0;
        int32 _durability = 0;
    };


    // 장비가 아닌 아이템은 ItemGear 행이 없어 세부 값이 NULL로 온다. 바인딩한 변수에는 NULL이 쓰이지 않으므로 0으로 바꿔 읽는다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SELECT i.ItemUid, i.TemplateId, i.ItemType, i.Container, i.SlotIndex, i.StackCount,
               ISNULL(g.EnhanceLevel, 0), ISNULL(g.Durability, 0)
        FROM dbo.Item AS i
        LEFT JOIN dbo.ItemGear AS g
            ON g.ItemUid = i.ItemUid
        WHERE i.OwnerId = (?)
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    while (conn.Fetch())
    {
        ItemRow row;
        row.itemUid = bindObject._itemUid;
        row.templateId = bindObject._templateId;
        row.itemType = static_cast<ItemRowType>(bindObject._itemType);
        row.container = static_cast<ItemRowContainer>(bindObject._container);
        row.slotIndex = bindObject._slotIndex;
        row.stackCount = bindObject._stackCount;
        row.enhanceLevel = bindObject._enhanceLevel;
        row.durability = bindObject._durability;

        // 둘 곳을 모르는 행을 건너뛰고 입장시키면 다음 저장이 그 행을 사본에 없는 행으로 보고 지운다.
        optional<PlacedItem> placed = ItemRows::ToPlacedItem(row);
        if (placed.has_value() == false)
        {
            throw DBError(__func__, format("아이템 {}의 자리(컨테이너 {}, 구역 {}, 칸 {})에 둘 수 없다",
                row.itemUid, bindObject._container, bindObject._itemType, row.slotIndex));
        }

        progress.items.push_back(std::move(placed.value()));
    }
}

void ItemDAO::SaveItems(DBConnection& conn, const PlayerSaveData& data)
{
    const int64 characterId = data.progress.characterId;
    vector<ItemRow> rows;
    rows.reserve(data.progress.items.size());
    for (const PlacedItem& placed : data.progress.items)
    {
        optional<ItemRow> row = ItemRows::ToRow(placed);
        if (row.has_value() == false)
        {
            throw DBError(__func__, format("행으로 저장할 수 없는 자리(컨테이너 {}, 칸 {})의 아이템(템플릿 {})",
                static_cast<int32>(placed.address.container), placed.address.slotIndex, placed.item.templateId));
        }

        // 행은 UID로 맞춘다. UID가 없으면 같은 아이템을 찾을 수 없다.
        if (row->itemUid <= 0)
            throw DBError(__func__, format("UID가 없는 아이템(템플릿 {}, 칸 {})", row->templateId, row->slotIndex));

        rows.push_back(row.value());
    }

    CreateItemSource(conn);

    // 배열 파라미터 한 번에 묶을 수 있는 행 수를 넘으면 나눠 넣는다. MERGE는 모든 행을 넣은 뒤 한 번만 실행해야
    // 사본에 없는 행을 가려낼 수 있다. 나눠 MERGE하면 앞 묶음을 지운다.
    for (size_t begin = 0; begin < rows.size(); begin += MAX_PARAM_ROWS)
    {
        const size_t end = (std::min)(begin + MAX_PARAM_ROWS, rows.size());
        const vector<ItemRow> chunk(rows.begin() + begin, rows.begin() + end);
        ExecuteParamSet<ItemRowsBinding>(conn, __func__, INSERT_ITEM_SOURCE, chunk);
    }

    MergeItemSource(conn, characterId);
}
