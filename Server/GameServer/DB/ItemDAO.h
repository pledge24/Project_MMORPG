#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 장비와 개수로 쌓이는 아이템(소모품, 기타)을 불러오고 저장한다.
 * GetMaxItemUID를 빼면 ProgressStorage만 부르고, DB 큐의 잡 안에서 블로킹으로 실행한다.
 * DB 작업이 실패하면 DBError를 던진다.
 */
class ItemDAO
{
public:
    /** 서버 시작 때 main 스레드에서 한 번 불러 GNextItemUID를 DB의 최대 UID 다음 값으로 정한다. */
    static void GetMaxItemUID(DBConnection& conn);

    /**
     * progress의 인벤토리와 착용 장비에 행마다 슬롯 하나를 붙인다. 슬롯 번호는 DB 행의 slot_id다.
     * 칸 수만큼 채운 배열이 아니므로, 칸에 넣는 일은 Player::ApplyProgress가 한다.
     */
    static void LoadItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    /** 더티 플래그가 켜진 슬롯만 반영한다. 가방의 더티 플래그가 없으면 DBError. */
    static void SaveItems(DBConnection& conn, const PlayerSaveData& data);

private:
    static void LoadGearItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    static void LoadStackableItems(DBConnection& conn, int64 characterId, Protocol::ItemType itemType, OUT PlayerProgress& progress);

    static void SaveGearItems(DBConnection& conn, const PlayerSaveData& data);
    static void SaveStackableItems(DBConnection& conn, const PlayerSaveData& data, Protocol::ItemType itemType);
};
