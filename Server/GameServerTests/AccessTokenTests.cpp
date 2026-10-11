#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/AccessToken.h"

/*--------------------------------------------------------------
    액세스 토큰 값 테스트

    로그인 잡은 Redis의 토큰 키를 읽고 지운 뒤 그 값으로 계정을 안다. 값의 형식이 틀리면 예외가 잡 밖으로 나가
    로그인 응답 없이 잡이 끝났다(TD-018). 토큰은 이미 지웠으므로 클라이언트는 응답을 기다리며 멈춘다.

    픽스처 결합도: 없음. 순수 함수만 부른다.
---------------------------------------------------------------*/

TEST(AccessToken, ReadsUserIdAndUsername)
{
    optional<AccessToken::Payload> payload = AccessToken::ParsePayload(R"({"userId": 42, "username": "tester"})");

    ASSERT_TRUE(payload.has_value());
    EXPECT_EQ(payload->userId, 42);
    EXPECT_EQ(payload->username, "tester");
}

TEST(AccessToken, MalformedValueIsRejected)
{
    EXPECT_EQ(AccessToken::ParsePayload("not json"), nullopt);
}

TEST(AccessToken, MissingFieldIsRejected)
{
    EXPECT_EQ(AccessToken::ParsePayload(R"({"username": "tester"})"), nullopt);
    EXPECT_EQ(AccessToken::ParsePayload(R"({"userId": 42})"), nullopt);
}

TEST(AccessToken, WrongFieldTypeIsRejected)
{
    EXPECT_EQ(AccessToken::ParsePayload(R"({"userId": "42", "username": "tester"})"), nullopt);
    EXPECT_EQ(AccessToken::ParsePayload(R"([42, "tester"])"), nullopt);
}
