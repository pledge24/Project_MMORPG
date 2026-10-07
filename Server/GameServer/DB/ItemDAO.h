#pragma once

struct PlayerSaveData;

/**
 * 장비와 개수로 쌓이는 아이템(소모품, 기타)을 불러오고 저장한다.
 * GetMaxItemUID를 빼면 ProgressStorage만 부르고, DB 큐의 잡 안에서 블로킹으로 실행한다.
 */
class ItemDAO
{
public:
    /** 서버 시작 때 main 스레드에서 한 번 불러 GNextItemUID를 DB의 최대 UID 다음 값으로 정한다. */
    static bool GetMaxItemUID();

    /** 세션 Player의 인벤토리와 착용 장비에 직접 넣는다. 세션에 Player가 있어야 한다. */
    static bool LoadItems(SessionRef session, int64 characterId);
    /** 더티 플래그가 켜진 슬롯만 반영한다. 가방의 더티 플래그가 없으면 실패로 끝낸다. */
    static bool SaveItems(const PlayerSaveData& data);

private:
    static bool LoadGearItems(SessionRef session, int64 characterId);
    static bool LoadStackableItems(SessionRef session, int64 characterId, Protocol::ItemType itemType);

    static bool SaveGearItems(const PlayerSaveData& data);
    static bool SaveStackableItems(const PlayerSaveData& data, Protocol::ItemType itemType);
};
