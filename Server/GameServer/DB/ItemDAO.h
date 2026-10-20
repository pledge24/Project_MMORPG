#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 아이템의 DB 접근 창구.
 *
 * 캐릭터가 가진 장비와 개수로 쌓이는 아이템(소모품, 기타)을 불러오고 저장한다.
 * DB 작업이 실패하면 DBError를 던진다.
 */
class ItemDAO
{
public:
    /**
     * 캐릭터의 아이템을 progress의 인벤토리와 착용 장비에 붙인다. DB 큐의 잡 안에서 부른다.
     * 행마다 슬롯을 하나씩 붙일 뿐이라 배열 순번은 슬롯 번호가 아니다. 슬롯 번호는 각 슬롯의 slot_id다.
     */
    static void LoadItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    /** 더티 플래그가 켜진 슬롯만 저장한다. DB 큐의 잡 안에서 부르고, 가방의 더티 플래그가 없으면 DBError. */
    static void SaveItems(DBConnection& conn, const PlayerSaveData& data);

private:
    static void LoadGearItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    static void LoadStackableItems(DBConnection& conn, int64 characterId, Protocol::ItemType itemType, OUT PlayerProgress& progress);

    static void SaveGearItems(DBConnection& conn, const PlayerSaveData& data);
    static void SaveStackableItems(DBConnection& conn, const PlayerSaveData& data, Protocol::ItemType itemType);
};
