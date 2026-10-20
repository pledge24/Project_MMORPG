#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 아이템의 DB 접근 창구.
 *
 * 캐릭터가 가진 아이템(Item)과 장비의 세부 값(ItemGear)을 불러오고 저장한다.
 * 모든 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
class ItemDAO
{
public:
    /**
     * 캐릭터의 아이템을 progress의 인벤토리와 착용 장비에 붙인다. 컨테이너나 구역을 모르는 행이 있으면 DBError.
     * 행마다 칸을 하나씩 붙일 뿐이라 배열 순번은 칸 번호가 아니다. 칸 번호는 각 칸의 slot_id다.
     */
    static void LoadItems(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    /**
     * 사본에 든 아이템 전체로 그 캐릭터의 아이템 행을 맞추고, 사본에 없는 행은 지운다.
     * 여러 문장을 차례로 실행하므로 트랜잭션 안에서 부르고, DBError를 받으면 되돌린다.
     */
    static void SaveItems(DBConnection& conn, const PlayerSaveData& data);
};
