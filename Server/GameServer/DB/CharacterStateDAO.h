#pragma once

struct PlayerSaveData;

/**
 * 캐릭터 기본 정보(Characters)와 마지막 상태(CharactersLastState)를 불러오고 저장한다.
 * ProgressStorage만 부른다. DB 큐의 잡 안에서 블로킹으로 실행한다.
 */
class CharacterStateDAO
{
public:
    //~ 불러오기
    /** 세션의 Player에 직접 채운다. 세션에 Player가 있어야 한다. 이 계정의 캐릭터가 아니면 false를 돌려준다. */
    static bool LoadCharacter(SessionRef session, int64 characterId);
    /** 세션의 Player에 직접 채운다. 세션에 Player가 있어야 한다. */
    static bool LoadLastState(SessionRef session, int64 characterId);

    //~ 저장
    /** 룸 큐에서 뜬 사본으로 저장한다. 살아 있는 Player는 읽지 않는다. */
    static bool SaveCharacter(const PlayerSaveData& data);
    static bool SaveLastState(const PlayerSaveData& data);
};
