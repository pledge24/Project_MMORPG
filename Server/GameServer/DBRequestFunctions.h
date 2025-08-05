#pragma once
class DBRequestFunctions
{
public:
    // 여기에 여러 DB 요청함수들을 넣으면 됨.
    static void GetCharacterData(SessionRef session, int64 userId);
    static void CreateCharacter(SessionRef session, const Protocol::CharacterOverview& character, int64 userId);
    static void DeleteCharacter(SessionRef session, int64 characterId);
};

