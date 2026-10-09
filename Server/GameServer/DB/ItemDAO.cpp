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

bool ItemDAO::GetMaxItemUID()
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
        int32 _maxItemUID;
    };

    DBConnectionGuard dbConn;

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
        return false;
    }

    return true;
}

bool ItemDAO::LoadItems(int64 characterId, OUT PlayerProgress& progress)
{
    try
    {
        // 1. 캐릭터 장비 아이템 가져오기
        if (LoadGearItems(characterId, progress) == false)
            throw wstring(L"장비 아이템 불러오기 실패");

        // 2. 캐릭터 소비 아이템 가져오기
        if (LoadStackableItems(characterId, Protocol::ITEM_TYPE_CONSUMABLE, progress) == false)
            throw wstring(L"소비 아이템 불러오기 실패");

        // 3. 캐릭터 기타 아이템 가져오기
        if (LoadStackableItems(characterId, Protocol::ITEM_TYPE_MISCELLANEOUS, progress) == false)
            throw wstring(L"기타 아이템 불러오기 실패");
    }
    catch (const wstring& cause)
    {
        wcout << cause << endl;
        return false;
    }

    return true;
}

bool ItemDAO::SaveItems(const PlayerSaveData& data)
{
    try
    {
        // 1. 캐릭터 장비 아이템 갱신하기
        if (SaveGearItems(data) == false)
            throw wstring(L"장비 아이템 저장 실패");

        // 2. 캐릭터 소비 아이템 갱신하기
        if (SaveStackableItems(data, Protocol::ITEM_TYPE_CONSUMABLE) == false)
            throw wstring(L"소비 아이템 저장 실패");

        // 3. 캐릭터 기타 아이템 갱신하기
        if (SaveStackableItems(data, Protocol::ITEM_TYPE_MISCELLANEOUS) == false)
            throw wstring(L"기타 아이템 저장 실패");
    }
    catch (const wstring& cause)
    {
        wcout << cause << endl;
        return false;
    }

    return true;
}

bool ItemDAO::LoadGearItems(int64 characterId, OUT PlayerProgress& progress)
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

    DBConnectionGuard dbConn;

    try
    {
        // 해당 캐릭터의 장비 아이템 정보를 가져온다.
        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
            SELECT item_uid, template_id, is_equipped, slot_id, enhance, durability, additional_physical_attack, additional_magical_attack
            FROM [dbo].[CharactersGearItems]
            WHERE character_id = (?)
        )SQL");

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        while (dbConn->Fetch())
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
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool ItemDAO::LoadStackableItems(int64 characterId, Protocol::ItemType itemType, OUT PlayerProgress& progress)
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
        return false;

    DBConnectionGuard dbConn;

    try
    {
        // 해당 캐릭터의 소비 아이템이나 기타 아이템 정보를 가져온다.
        // DBBind는 쿼리 문자열을 가리키기만 하므로 문자열이 실행보다 오래 살아야 한다.
        const wstring query = wstring(LR"SQL(
            SELECT slot_id, template_id, count
            FROM )SQL") + table + LR"SQL(
            WHERE character_id = (?)
        )SQL";
        DBBind<PARAMS, COLS> dbBind(*dbConn, query.c_str());

        BindObject bindObject(dbBind, characterId);

        if (dbBind.Execute() == false)
            throw DBCustomError::SQL_EXECUTE_FAIL;

        Protocol::Inventory* inventory = progress.possession.mutable_inventory();
        const bool isConsumable = itemType == Protocol::ITEM_TYPE_CONSUMABLE;

        while (dbConn->Fetch())
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
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool ItemDAO::SaveGearItems(const PlayerSaveData& data)
{
    const int PARAMS = 9;
    const int COLS = 0;
    const int MAX_ROWS = 100;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const vector<GearSaveRow>& rows)
        {
            // MemSet
            ::memset(_characterId, 0, sizeof(_characterId));
            ::memset(_slotId, 0, sizeof(_slotId));
            ::memset(_itemUid, 0, sizeof(_itemUid));
            ::memset(_templateId, 0, sizeof(_templateId));
            ::memset(_isEquipped, false, sizeof(_isEquipped));
            ::memset(_enhance, 0, sizeof(_enhance));
            ::memset(_durability, 0, sizeof(_durability));
            ::memset(_additionalPhysicalAttack, 0, sizeof(_additionalPhysicalAttack));
            ::memset(_additionalMagicalAttack, 0, sizeof(_additionalMagicalAttack));

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

            if (rowCount > 0)
                BindParam(dbBind, rowCount);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind, int32 rows)
        {
            dbBind.BindParamSet(0, _characterId, rows);
            dbBind.BindParamSet(1, _slotId, rows);
            dbBind.BindParamSet(2, _itemUid, rows);
            dbBind.BindParamSet(3, _templateId, rows);
            dbBind.BindParamSet(4, _isEquipped, rows);
            dbBind.BindParamSet(5, _enhance, rows);
            dbBind.BindParamSet(6, _durability, rows);
            dbBind.BindParamSet(7, _additionalPhysicalAttack, rows);
            dbBind.BindParamSet(8, _additionalMagicalAttack, rows);
        }

        //~ Params
        int64 _characterId[MAX_ROWS];
        int32 _slotId[MAX_ROWS];
        int64 _itemUid[MAX_ROWS];
        int32 _templateId[MAX_ROWS];
        bool _isEquipped[MAX_ROWS];
        int32 _enhance[MAX_ROWS];
        int32 _durability[MAX_ROWS];
        int32 _additionalPhysicalAttack[MAX_ROWS];
        int32 _additionalMagicalAttack[MAX_ROWS];
    };

    DBConnectionGuard dbConn;

    try
    {
        // 더티 플래그가 없으면 이 요청을 실패로 끝낸다. 빈 결과로 넘기면 아무것도 반영하지 않고 성공으로 보고한다.
        optional<vector<GearSaveRow>> rows = ItemSaveRows::BuildGearRows(data);
        if (rows.has_value() == false)
            throw DBCustomError::INVENTORY_DIRTY_FLAGS_NOT_FOUND;

        DBBind<PARAMS, COLS> dbBind(*dbConn, LR"SQL(
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
        )SQL");

        BindObject bindObject(dbBind, rows.value());

        int32 rowCount = static_cast<int32>(rows->size());
        if (rowCount > 0)
        {
            dbConn->SetParamSetSize(rowCount);

            if (dbBind.Execute() == false)
                throw DBCustomError::SQL_EXECUTE_FAIL;
        }
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}

bool ItemDAO::SaveStackableItems(const PlayerSaveData& data, Protocol::ItemType itemType)
{
    const int PARAMS = 4;
    const int COLS = 0;
    const int MAX_ROWS = 100;

    struct BindObject
    {
        BindObject(DBBind<PARAMS, COLS>& dbBind, const vector<StackableItemSaveRow>& rows)
        {
            // MemSet
            ::memset(_characterId, 0, sizeof(_characterId));
            ::memset(_slotId, 0, sizeof(_slotId));
            ::memset(_templateId, 0, sizeof(_templateId));
            ::memset(_count, 0, sizeof(_count));

            const int32 rowCount = static_cast<int32>(rows.size());
            for (int32 i = 0; i < rowCount; i++)
            {
                const StackableItemSaveRow& row = rows[i];
                _characterId[i] = row.characterId;
                _slotId[i] = row.slotId;
                _templateId[i] = row.templateId;
                _count[i] = row.count;
            }

            if (rowCount > 0)
                BindParam(dbBind, rowCount);
        }

        void BindParam(DBBind<PARAMS, COLS>& dbBind, int32 rows)
        {
            dbBind.BindParamSet(0, _characterId, rows);
            dbBind.BindParamSet(1, _slotId, rows);
            dbBind.BindParamSet(2, _templateId, rows);
            dbBind.BindParamSet(3, _count, rows);
        }

        //~ Params
        int64 _characterId[MAX_ROWS];
        int32 _slotId[MAX_ROWS];
        int32 _templateId[MAX_ROWS];
        int32 _count[MAX_ROWS];
    };

    const WCHAR* table = GetStackableItemTable(itemType);
    if (table == nullptr)
        return false;

    DBConnectionGuard dbConn;

    try
    {
        // 더티 플래그가 없으면 이 요청을 실패로 끝낸다. 빈 결과로 넘기면 아무것도 반영하지 않고 성공으로 보고한다.
        optional<vector<StackableItemSaveRow>> rows = ItemSaveRows::BuildStackableRows(data, itemType);
        if (rows.has_value() == false)
            throw DBCustomError::INVENTORY_DIRTY_FLAGS_NOT_FOUND;

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
        DBBind<PARAMS, COLS> dbBind(*dbConn, query.c_str());

        BindObject bindObject(dbBind, rows.value());

        int32 rowCount = static_cast<int32>(rows->size());
        if (rowCount > 0)
        {
            dbConn->SetParamSetSize(rowCount);

            if (dbBind.Execute() == false)
                throw DBCustomError::SQL_EXECUTE_FAIL;
        }
    }
    catch (DBCustomError error)
    {
        PrintDBErrorLog(error);
        return false;
    }

    return true;
}
