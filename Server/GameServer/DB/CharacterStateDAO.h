#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 기본 정보와 마지막 상태의 DB 접근 창구.
 *
 * 캐릭터의 이름, 레벨 같은 기본 정보와 접속을 끊을 때의 상태, 골드를 불러오고 저장한다.
 * 모든 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
class CharacterStateDAO
{
    //~ 불러오기(Load)
public:
    /**
     * 기본 정보, 마지막 맵과 위치, 누적 경험치, HP와 MP, 골드를 progress에 채우고 마지막 입장 시각을 갱신한다.
     * userId 계정의 삭제되지 않은 캐릭터가 아니면 아무것도 바꾸지 않고 false. 골드 행이 없으면 DBError.
     */
    static bool LoadCharacter(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress);

    //~ 저장(Save)
public:
    /** 목록에 보일 레벨 사본을 저장한다. 갱신한 행이 하나가 아니면 DBError. */
    static void SaveCharacter(DBConnection& conn, const PlayerSaveData& data);
    /** 누적 경험치, 맵과 위치, HP와 MP를 저장한다. 갱신한 행이 하나가 아니면 DBError. */
    static void SaveState(DBConnection& conn, const PlayerSaveData& data);
    /** 갱신한 행이 하나가 아니면 DBError. */
    static void SaveGold(DBConnection& conn, const PlayerSaveData& data);
};
