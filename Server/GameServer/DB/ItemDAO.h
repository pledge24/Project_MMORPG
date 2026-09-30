#pragma once

struct PlayerSaveData;

/*-------------------------
    ItemDAO

    장비와 개수로 쌓이는 아이템(소모품, 기타)을 불러오고 저장한다.
--------------------------*/

class ItemDAO
{
public:
    // 서버 시작 때 다음 아이템 UID를 정한다.
    static bool GetMaxItemUID();

    static bool LoadItems(SessionRef session, int64 characterId);
    static bool SaveItems(const PlayerSaveData& data);

    static bool LoadGearItems(SessionRef session, int64 characterId);
    static bool LoadStackableItems(SessionRef session, int64 characterId, Protocol::ItemType itemType);

    static bool SaveGearItems(const PlayerSaveData& data);
    static bool SaveStackableItems(const PlayerSaveData& data, Protocol::ItemType itemType);
};
