#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 캐릭터 기본 정보(Characters)와 마지막 상태(CharactersLastState)를 불러오고 저장한다.
 * ProgressStorage만 부른다. DB 큐의 잡 안에서 블로킹으로 실행한다.
 */
class CharacterStateDAO
{
public:
    //~ 불러오기(Load)
    /** 캐릭터 번호, 직업, 이름, 레벨을 progress에 채운다. userId 계정의 캐릭터가 아니면 false를 돌려준다. */
    static bool LoadCharacter(int64 userId, int64 characterId, OUT PlayerProgress& progress);
    /** 경험치, 현재 HP와 MP, 공격력, 마지막 맵과 룸과 위치, 골드를 progress에 채운다. */
    static bool LoadLastState(int64 characterId, OUT PlayerProgress& progress);

    //~ 저장(Save)
    /** 룸 큐에서 뜬 사본으로 저장한다. 살아 있는 Player는 읽지 않는다. */
    static bool SaveCharacter(const PlayerSaveData& data);
    static bool SaveLastState(const PlayerSaveData& data);
};
