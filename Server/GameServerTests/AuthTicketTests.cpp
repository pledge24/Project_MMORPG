#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/AuthTicket.h"

/*--------------------------------------------------------------
    인증 티켓 값 테스트

    로그인 잡은 Redis의 인증 티켓 키를 읽고 지운 뒤 그 값으로 계정을 안다. 티켓은 이미 지웠으므로 값을
    잘못 읽으면 다른 계정으로 로그인되거나, 예외가 잡 밖으로 나가 클라이언트가 응답을 기다리며 멈춘다.

    픽스처 결합도: 없음. 순수 함수만 부른다.
---------------------------------------------------------------*/

TEST(AuthTicket, ReadsAccountId)
{
    EXPECT_EQ(AuthTicket::ParseAccountId("42"), optional<int64>(42));
}

TEST(AuthTicket, ReadsInt64Max)
{
    // 인증 서버는 BIGINT 계정 번호를 숫자로 바꾸지 않고 문자열 그대로 넣는다. 2^53을 넘어도 정확해야 한다.
    EXPECT_EQ(AuthTicket::ParseAccountId("9223372036854775807"), optional<int64>(INT64_MAX));
}

TEST(AuthTicket, EmptyValueIsRejected)
{
    EXPECT_EQ(AuthTicket::ParseAccountId(""), nullopt);
}

TEST(AuthTicket, NonPositiveValueIsRejected)
{
    EXPECT_EQ(AuthTicket::ParseAccountId("0"), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId("-1"), nullopt);
}

TEST(AuthTicket, NonCanonicalValueIsRejected)
{
    EXPECT_EQ(AuthTicket::ParseAccountId("042"), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId("+42"), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId(" 42"), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId("42 "), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId("4x"), nullopt);
}

TEST(AuthTicket, ValueBeyondInt64IsRejected)
{
    EXPECT_EQ(AuthTicket::ParseAccountId("9223372036854775808"), nullopt);
    EXPECT_EQ(AuthTicket::ParseAccountId("99999999999999999999"), nullopt);
}

TEST(AuthTicket, OldJsonValueIsRejected)
{
    EXPECT_EQ(AuthTicket::ParseAccountId(R"({"userId": 42, "username": "tester"})"), nullopt);
}
