#include "Core/pch.h"
#include "DB/ItemDAO.h"
#include "DB/DAOCommon.h"
#include "DB/ItemSaveRows.h"
#include "Game/Entities/PlayerProgress.h"

namespace
{
    // 불러온 행 하나를 슬롯으로 만든다. 칸 번호는 slot_id에 싣는다.
    Protocol::Slot MakeLoadedSlot(Protocol::SlotType slotType, int32 slotId, const Protocol::Item& item)
    {
        Protocol::Slot slot;
        slot.set_type(slotType);
        slot.set_slot_id(slotId);
        *slot.mutable_item() = item;
        return slot;
    }

    // 장비 행을 배열 파라미터로 묶는다. 순서는 저장 쿼리의 물음표 순서다.
    struct GearRowsBinding
    {
        static constexpr int32 PARAMS = 9;

        GearRowsBinding(DBBind<PARAMS, 0>& dbBind, const vector<GearSaveRow>& rows)
        {
            const int32 rowCount = static_cast<int32>(rows.size());
            for (int32 i = 0; i < rowCount; i++)
            {
                const GearSaveRow& row = rows[i];
                _characterId[i] = row.characterId;
                _slotId[i] = row.slotId;
                _itemUid[i] = row.itemUid;
                _templateId[i] = row.templateId;
                _isEquipped[i] = row.isEquipped;
                _enhance[i] = row.enhance;
                _durability[i] = row.durability;
                _additionalPhysicalAttack[i] = row.additionalPhysicalAttack;
                _additionalMagicalAttack[i] = row.additionalMagicalAttack;
            }

            dbBind.BindParamSet(0, _characterId, rowCount);
            dbBind.BindParamSet(1, _slotId, rowCount);
            dbBind.BindParamSet(2, _itemUid, rowCount);
            dbBind.BindParamSet(3, _templateId, rowCount);
            dbBind.BindParamSet(4, _isEquipped, rowCount);
            dbBind.BindParamSet(5, _enhance, rowCount);
            dbBind.BindParamSet(6, _durability, rowCount);
            dbBind.BindParamSet(7, _additionalPhysicalAttack, rowCount);
            dbBind.BindParamSet(8, _additionalMagicalAttack, rowCount);
        }

        int64 _characterId[MAX_PARAM_ROWS] = {};
        int32 _slotId[MAX_PARAM_ROWS] = {};
        int64 _itemUid[MAX_PARAM_ROWS] = {};
        int32 _templateId[MAX_PARAM_ROWS] = {};
        bool _isEquipped[MAX_PARAM_ROWS] = {};
        int32 _enhance[MAX_PARAM_ROWS] = {};
        int32 _durability[MAX_PARAM_ROWS] = {};
        int32 _additionalPhysicalAttack[MAX_PARAM_ROWS] = {};
        int32 _additionalMagicalAttack[MAX_PARAM_ROWS] = {};
    };

    // 소모품이나 기타 아이템 행을 배열 파라미터로 묶는다. 순서는 저장 쿼리의 물음표 순서다.
    struct StackableRowsBinding
    {
        static constexpr int32 PARAMS = 4;

        StackableRowsBinding(DBBind<PARAMS, 0>& dbBind, const vector<StackableItemSaveRow>& rows)
        {
            const int32 rowCount = static_cast<int32>(rows.size());
            for (int32 i = 0; i < rowCount; i++)
            {
                const StackableItemSaveRow& row = rows[i];
                _characterId[i] = row.characterId;
                _slotId[i] = row.slotId;
                _templateId[i] = row.templateId;
                _count[i] = row.count;
            }

            dbBind.BindParamSet(0, _characterId, rowCount);
            dbBind.BindParamSet(1, _slotId, rowCount);
            dbBind.BindParamSet(2, _templateId, rowCount);
            dbBind.BindParamSet(3, _count, rowCount);
        }

        int64 _characterId[MAX_PARAM_ROWS] = {};
        int32 _slotId[MAX_PARAM_ROWS] = {};
        int32 _templateId[MAX_PARAM_ROWS] = {};
        int32 _count[MAX_PARAM_ROWS] = {};
    };

    // 개수로 쌓이는 아이템의 테이블이다. 쌓이는 종류가 아니면 nullptr.
    const WCHAR* GetStackableItemTable(Protocol::ItemType itemType)
    {
        switch (itemType)
        {
        case Protocol::ITEM_TYPE_CONSUMABLE:
            return L"[dbo].[CharactersConsumableItems]";
        case Protocol::ITEM_TYPE_MISCELLANEOUS:
            return L"[dbo].[CharactersMiscItems]";
        default:
            return nullptr;
        }
    }
}

void ItemDAO::GetMaxItemUID(DBConnection& conn)
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

        //~ Cols
        // item_uid는 BIGINT다. int32로 받으면 범위를 넘을 때 잘린다.
        int64 _maxItemUID = 0;
    };


    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        EXEC GetMaxItemUID;
    )SQL");

    BindObject bindObject(dbBind);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    if (conn.Fetch() == false)
        throw DBError(__func__, "결과 행이 없다");

    GNextItemUID = bindObject._maxItemUID + 1; // itemUid저장
}

void ItemDAO::LoadItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress)
{
    LoadGearItems(conn, characterId, progress);
    LoadStackableItems(conn, characterId, Protocol::ITEM_TYPE_CONSUMABLE, progress);
    LoadStackableItems(conn, characterId, Protocol::ITEM_TYPE_MISCELLANEOUS, progress);
}

void ItemDAO::SaveItems(DBConnection& conn, const PlayerSaveData& data)
{
    SaveGearItems(conn, data);
    SaveStackableItems(conn, data, Protocol::ITEM_TYPE_CONSUMABLE);
    SaveStackableItems(conn, data, Protocol::ITEM_TYPE_MISCELLANEOUS);
}

void ItemDAO::LoadGearItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress)
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

        //~ Params
        int64 _characterId;

        //~ Cols
        int64 _itemUid;
        int32 _templateId;
        bool _isEquipped;
        int32 _slotId;
        int32 _enhance;
        int32 _durability;
        int32 _additionalPhysicalAttack;
        int32 _additionalMagicalAttack;
    };


    // 해당 캐릭터의 장비 아이템 정보를 가져온다.
    DBBind<PARAMS, COLS> dbBind(conn, LR"SQL(
        SELECT item_uid, template_id, is_equipped, slot_id, enhance, durability, additional_physical_attack, additional_magical_attack
        FROM [dbo].[CharactersGearItems]
        WHERE character_id = (?)
    )SQL");

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    while (conn.Fetch())
    {
        Protocol::Item item;
        Protocol::GearInfo* gearInfo = item.mutable_gearinfo();

        item.set_template_id(bindObject._templateId);
        item.set_item_uid(bindObject._itemUid);
        item.set_count(1);

        gearInfo->set_enhance_level(bindObject._enhance);
        gearInfo->set_durability(bindObject._durability);
        gearInfo->set_additional_physical_attack(bindObject._additionalPhysicalAttack);
        gearInfo->set_additional_magical_attack(bindObject._additionalMagicalAttack);

        if (bindObject._isEquipped == false)
            *progress.possession.mutable_inventory()->add_gear() = MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_GEAR, bindObject._slotId, item);
        else
            (*progress.possession.mutable_equipped_gear())[bindObject._slotId] = MakeLoadedSlot(Protocol::SLOT_TYPE_EQUIPPED, bindObject._slotId, item);
    }
}

void ItemDAO::LoadStackableItems(DBConnection& conn, int64 characterId, Protocol::ItemType itemType, OUT PlayerProgress& progress)
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

        //~ Params
        int64 _characterId;

        //~ Cols
        int32 _slotId;
        int32 _templateId;
        int32 _count;
    };

    const WCHAR* table = GetStackableItemTable(itemType);
    if (table == nullptr)
        throw DBError(__func__, "쌓이는 아이템 종류가 아니다");


    // 해당 캐릭터의 소비 아이템이나 기타 아이템 정보를 가져온다.
    // DBBind는 쿼리 문자열을 가리키기만 하므로 문자열이 실행보다 오래 살아야 한다.
    const wstring query = wstring(LR"SQL(
        SELECT slot_id, template_id, count
        FROM )SQL") + table + LR"SQL(
        WHERE character_id = (?)
    )SQL";
    DBBind<PARAMS, COLS> dbBind(conn, query.c_str());

    BindObject bindObject(dbBind, characterId);

    if (dbBind.Execute() == false)
        throw DBError(__func__, "쿼리 실행 실패");

    Protocol::Inventory* inventory = progress.possession.mutable_inventory();
    const bool isConsumable = itemType == Protocol::ITEM_TYPE_CONSUMABLE;

    while (conn.Fetch())
    {
        Protocol::Item item;
        item.set_template_id(bindObject._templateId);
        item.set_count(bindObject._count);

        if (isConsumable)
            *inventory->add_consumables() = MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, bindObject._slotId, item);
        else
            *inventory->add_miscellaneous() = MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_MISC, bindObject._slotId, item);
    }
}

void ItemDAO::SaveGearItems(DBConnection& conn, const PlayerSaveData& data)
{


    // 더티 플래그가 없으면 이 요청을 실패로 끝낸다. 빈 결과로 넘기면 아무것도 반영하지 않고 성공으로 보고한다.
    optional<vector<GearSaveRow>> rows = ItemSaveRows::BuildGearRows(data);
    if (rows.has_value() == false)
        throw DBError(__func__, "가방의 더티 플래그가 없다");

    const WCHAR* query = LR"SQL(
        -- 1. 임시 테이블 생성
        SELECT *
        INTO #TempTable
        FROM [dbo].[CharactersGearItems]
        WHERE 1 = 0;

        -- 2. 임시 테이블에 INSERT
        INSERT INTO #TempTable (character_id, slot_id, item_uid, template_id, is_equipped, enhance, durability, additional_physical_attack, additional_magical_attack)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);

        -- 3. MERGE 진행
        MERGE INTO [dbo].[CharactersGearItems] AS T
        USING #TempTable AS S
        ON (T.character_id = S.character_id
            AND T.slot_id = S.slot_id
            AND T.is_equipped = S.is_equipped)

        -- 3-1. 매칭된 행이면서 #TempTable의 template_id = 0 -> DELETE
        WHEN MATCHED AND S.template_id = 0 THEN
            DELETE

        -- 3-2. 매칭된 행이면서 #TempTable의 template_id > 0 -> UPDATE
        WHEN MATCHED THEN
            UPDATE SET
                item_uid = S.item_uid,
                template_id = S.template_id,
                is_equipped = S.is_equipped,
                enhance = S.enhance,
                durability = S.durability,
                additional_physical_attack = S.additional_physical_attack,
                additional_magical_attack = S.additional_magical_attack

        -- 3-3. DB 테이블에 행이 없으면서 #TempTable의 template_id > 0 -> INSERT
        WHEN NOT MATCHED BY TARGET AND S.template_id > 0 THEN
            INSERT (character_id, slot_id, item_uid, template_id, is_equipped, enhance, durability, additional_physical_attack, additional_magical_attack)
            VALUES (S.character_id, S.slot_id, S.item_uid, S.template_id, S.is_equipped, S.enhance, S.durability, S.additional_physical_attack, S.additional_magical_attack);

        -- 4. 임시 테이블 삭제
        DROP TABLE #TempTable;
    )SQL";

    ExecuteParamSet<GearRowsBinding>(conn, __func__, query, rows.value());
}

void ItemDAO::SaveStackableItems(DBConnection& conn, const PlayerSaveData& data, Protocol::ItemType itemType)
{

    const WCHAR* table = GetStackableItemTable(itemType);
    if (table == nullptr)
        throw DBError(__func__, "쌓이는 아이템 종류가 아니다");

    // 더티 플래그가 없으면 이 요청을 실패로 끝낸다. 빈 결과로 넘기면 아무것도 반영하지 않고 성공으로 보고한다.
    optional<vector<StackableItemSaveRow>> rows = ItemSaveRows::BuildStackableRows(data, itemType);
    if (rows.has_value() == false)
        throw DBError(__func__, "가방의 더티 플래그가 없다");

    // 해당 캐릭터의 소비 아이템이나 기타 아이템 정보를 갱신한다.
    // DBBind는 쿼리 문자열을 가리키기만 하므로 문자열이 실행보다 오래 살아야 한다.
    // MERGE는 세미콜론으로 끝나야 한다.
    const wstring query = wstring(LR"SQL(
        -- 1. 임시 테이블 생성
        SELECT *
        INTO #TempTable
        FROM )SQL") + table + LR"SQL(
        WHERE 1 = 0;

        -- 2. 임시 테이블에 INSERT
        INSERT INTO #TempTable (character_id, slot_id, template_id, count)
        VALUES (?, ?, ?, ?);

        -- 3. MERGE 진행
        MERGE INTO )SQL" + table + LR"SQL( AS T
        USING #TempTable AS S
        ON (T.character_id = S.character_id AND T.slot_id = S.slot_id)

        -- 3-1. 매칭된 행이면서 #TempTable의 template_id = 0 -> DELETE
        WHEN MATCHED AND S.template_id = 0 THEN
            DELETE

        -- 3-2. 매칭된 행이면서 #TempTable의 template_id > 0 -> UPDATE
        WHEN MATCHED THEN
            UPDATE SET
                template_id = S.template_id,
                count = S.count

        -- 3-3. DB 테이블에 행이 없으면서 #TempTable의 template_id > 0 -> INSERT
        WHEN NOT MATCHED BY TARGET AND S.template_id > 0 THEN
            INSERT (character_id, slot_id, template_id, count)
            VALUES (S.character_id, S.slot_id, S.template_id, S.count);

        -- 4. 임시 테이블 삭제
        DROP TABLE #TempTable;
    )SQL";

    ExecuteParamSet<StackableRowsBinding>(conn, __func__, query.c_str(), rows.value());
}
