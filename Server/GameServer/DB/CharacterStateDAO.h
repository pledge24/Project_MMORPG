#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 기본 정보와 마지막 상태의 DB 접근 창구.
 *
 * 캐릭터의 이름, 레벨 같은 기본 정보와 접속을 끊을 때의 상태를 불러오고 저장한다.
 * 모든 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
class CharacterStateDAO
{
    //~ 불러오기(Load)
public:
    /** 캐릭터 번호, 직업, 이름, 레벨을 progress에 채운다. userId 계정의 캐릭터가 아니면 false. */
    static bool LoadCharacter(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress);
    /** 스탯, 마지막 맵과 위치, 골드를 progress에 채운다. 행이 없어도 DBError. */
    static void LoadLastState(DBConnection& conn, int64 characterId, OUT PlayerProgress& progress);

    //~ 저장(Save)
public:
    static void SaveCharacter(DBConnection& conn, const PlayerSaveData& data);
    /** 갱신한 행이 하나가 아니어도 DBError. */
    static void SaveLastState(DBConnection& conn, const PlayerSaveData& data);
};
