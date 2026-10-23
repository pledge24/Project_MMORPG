#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/AuthTicket.h"

namespace
{
    // 인증 서버가 넣은 키를 흉내 내는 메모리 저장소. 지운 키는 다시 읽히지 않는다.
    class MemoryAuthTicketStore : public AuthTicketStore
    {
    public:
        optional<string> Get(const string& key) override
        {
            auto it = _values.find(key);
            if (it == _values.end())
                return nullopt;
            return it->second;
        }

        bool Delete(const string& key) override
        {
            return _values.erase(key) > 0;
        }

        map<string, string> _values;
    };
}

/*--------------------------------------------------------------
    인증 티켓 값 테스트

    로비 접속 잡은 Redis의 인증 티켓 키를 읽고 지운 뒤 그 값으로 계정을 안다. 티켓은 이미 지웠으므로 값을
    잘못 읽으면 다른 계정으로 접속되거나, 예외가 잡 밖으로 나가 클라이언트가 응답을 기다리며 멈춘다.
    티켓은 월드 선택이 그 월드에만 발급하므로, 다른 월드에 발급한 티켓으로는 이 서버의 로비에 들어오지 못해야 한다.

    픽스처 결합도: 없음. 순수 함수와 메모리 티켓 저장소만 쓴다.
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

// 키 형식은 인증 서버의 티켓 저장소가 쓰는 키와 같아야 한다. 어긋나면 모든 로비 접속이 거절된다.
TEST(AuthTicket, KeyHasWorldId)
{
    EXPECT_EQ(AuthTicket::MakeKey(3, "abc"), "authTicket:3:abc");
}

TEST(AuthTicket, ConsumesTicketOfThisWorld)
{
    MemoryAuthTicketStore store;
    store._values["authTicket:1:abc"] = "42";

    EXPECT_EQ(AuthTicket::Consume(store, 1, "abc"), optional<int64>(42));
    EXPECT_TRUE(store._values.empty()) << "소비한 티켓은 지운다";
}

TEST(AuthTicket, TicketOfOtherWorldIsRejected)
{
    MemoryAuthTicketStore store;
    store._values["authTicket:2:abc"] = "42";

    EXPECT_EQ(AuthTicket::Consume(store, 1, "abc"), nullopt);
    EXPECT_EQ(store._values.count("authTicket:2:abc"), 1u) << "다른 월드의 티켓은 그 월드가 쓸 수 있게 남긴다";
}

TEST(AuthTicket, TicketIsConsumedOnce)
{
    MemoryAuthTicketStore store;
    store._values["authTicket:1:abc"] = "42";

    ASSERT_TRUE(AuthTicket::Consume(store, 1, "abc").has_value());
    EXPECT_EQ(AuthTicket::Consume(store, 1, "abc"), nullopt);
}

TEST(AuthTicket, MissingTicketIsRejected)
{
    MemoryAuthTicketStore store;

    EXPECT_EQ(AuthTicket::Consume(store, 1, "abc"), nullopt);
}

// 티켓은 이미 지웠으므로 값을 읽지 못하면 거절한다. 그대로 두면 다시 쓸 수 없는 티켓이 계정 없이 통과한다.
TEST(AuthTicket, MalformedValueIsRejectedAfterDelete)
{
    MemoryAuthTicketStore store;
    store._values["authTicket:1:abc"] = "042";

    EXPECT_EQ(AuthTicket::Consume(store, 1, "abc"), nullopt);
    EXPECT_TRUE(store._values.empty());
}
