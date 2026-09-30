#pragma once

/*-------------------------
    CharacterListDAO

    계정의 캐릭터 목록을 불러오고, 캐릭터를 만들고 지운다. 결과 패킷을 세션에 보낸다.
--------------------------*/

class CharacterListDAO
{
public:
    static void LoadCharacterList(SessionRef session, int64 userId);
    static void CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId);
    static void DeleteCharacter(SessionRef session, int64 characterId);
};
