#pragma once

/*-------------------------
    DBRequestFunctions
--------------------------*/

class DBRequestFunctions
{
public:
    // 여기에 여러 DB 요청함수들을 넣으면 됨.
    static void GetUserCharactersData(SessionRef session, int64 userId);
    static void CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId);
    static void DeleteCharacter(SessionRef session, int64 characterId);
    static void GetEnterGameData(SessionRef session, int64 characterId);

private:
    /* GetEnterGameData가 호출*/
    static bool GetCharacterData(SessionRef session, int64 characterId);
    static bool GetCharacterLastStateData(SessionRef session, int64 characterId);
    static bool GetAllCharacterItems(SessionRef session, int64 characterId);

    /* GetAllCharacterItems가 호출*/
    static bool GetCharactersGearItems(SessionRef session, int64 characterId);
    static bool GetCharactersConsumableItems(SessionRef session, int64 characterId);
    static bool GetCharactersMiscItems(SessionRef session, int64 characterId);
};

