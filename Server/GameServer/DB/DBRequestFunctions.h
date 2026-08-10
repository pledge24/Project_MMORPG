#pragma once

/*-------------------------
    DBRequestFunctions
--------------------------*/

class DBRequestFunctions
{
public:
    // 여기에 여러 DB 요청함수들을 넣으면 됨.
    static void LoadUserCharactersData(SessionRef session, int64 userId);
    static void CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId);
    static void DeleteCharacter(SessionRef session, int64 characterId);

    static void LoadAllCharactersData(SessionRef session, int64 characterId);
    static void UpdateAllCharactersData(SessionRef session);

    static bool GetMaxItemUID();

private:
    /* LoadAllCharactersData가 호출 */
    static bool LoadCharacterData(SessionRef session, int64 characterId);
    static bool LoadCharacterLastStateData(SessionRef session, int64 characterId);
    static bool LoadAllCharacterItems(SessionRef session, int64 characterId);

    /* LoadAllCharacterItems가 호출 */
    static bool LoadCharactersGearItems(SessionRef session, int64 characterId);
    static bool LoadCharactersConsumableItems(SessionRef session, int64 characterId);
    static bool LoadCharactersMiscItems(SessionRef session, int64 characterId);

    /* UpdateAllCharactersData가 호출 */
    static bool UpdateCharacterData(SessionRef session);
    static bool UpdateCharacterLastStateData(SessionRef session);
    static bool UpdateAllCharacterItems(SessionRef session);

    /* UpdateAllCharacterItems가 호출 */
    static bool UpdateCharactersGearItems(SessionRef session);
    static bool UpdateCharactersConsumableItems(SessionRef session);
    static bool UpdateCharactersMiscItems(SessionRef session);
};

