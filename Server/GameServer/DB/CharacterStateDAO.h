#pragma once

struct PlayerSaveData;

/*-------------------------
    CharacterStateDAO

    캐릭터 기본 정보(Characters)와 마지막 상태(CharactersLastState)를 불러오고 저장한다.
--------------------------*/

class CharacterStateDAO
{
public:
    static bool LoadCharacter(SessionRef session, int64 characterId);
    static bool LoadLastState(SessionRef session, int64 characterId);

    static bool SaveCharacter(const PlayerSaveData& data);
    static bool SaveLastState(const PlayerSaveData& data);
};
