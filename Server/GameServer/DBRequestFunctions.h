#pragma once
class DBRequestFunctions
{
public:
    // 여기에 여러 DB 요청함수들을 넣으면 됨.
    static void GetCharacterData(int userId, string accessToken, SessionRef session);
};

