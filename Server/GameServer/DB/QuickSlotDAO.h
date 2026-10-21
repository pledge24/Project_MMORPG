#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 퀵슬롯의 DB 접근 창구.
 *
 * 캐릭터가 퀵슬롯에 등록한 대상(QuickSlot)을 불러오고 저장한다. 빈 퀵슬롯은 행이 없다.
 * 모든 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
class QuickSlotDAO
{
public:
    /** 캐릭터의 퀵슬롯 행을 progress의 퀵슬롯에 붙인다. 행의 값은 검증하지 않으므로 등록할 때 거른다. */
    static void LoadQuickSlots(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);
    /**
     * 사본의 퀵슬롯 전체로 그 캐릭터의 퀵슬롯 행을 맞추고, 사본에 없는 행은 지운다.
     * 여러 문장을 차례로 실행하므로 트랜잭션 안에서 부르고, DBError를 받으면 되돌린다.
     */
    static void SaveQuickSlots(DBConnection& conn, const PlayerSaveData& data);
};
