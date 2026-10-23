#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/ChatRequests.h"

/*--------------------------------------------------------------
    채팅 메시지 검증 테스트

    채팅은 말한 플레이어에게 결과 응답을, 룸의 모든 플레이어에게 메시지 통지를 보낸다.
    비었거나 길이 제한을 넘는 메시지는 룸에 중계하지 않고 결과 응답으로만 거절한다.
    길이는 UTF-16 글자 수로 센다. 한글 한 글자가 UTF-8로 3바이트라 바이트로 세면 한글 메시지만 짧아진다.

    픽스처 결합도: 없음. 검증은 문자열만 읽는다.
---------------------------------------------------------------*/

namespace
{
    // UTF-16 기준 count자인 한글 메시지를 UTF-8로 만든다.
    string MakeHangulMessage(int32 count)
    {
        string message;
        for (int32 i = 0; i < count; i++)
            message += "가";
        return message;
    }
}

TEST(ChatRequestsTest, OrdinaryMessageIsAccepted)
{
    EXPECT_EQ(ChatRequests::Validate("안녕하세요"), nullopt);
}

TEST(ChatRequestsTest, EmptyMessageIsRejected)
{
    EXPECT_EQ(ChatRequests::Validate(""), Protocol::CHAT_RESULT_INVALID_MESSAGE);
}

TEST(ChatRequestsTest, LengthLimitCountsUtf16Characters)
{
    EXPECT_EQ(ChatRequests::Validate(MakeHangulMessage(ChatRequests::MAX_MESSAGE_LENGTH)), nullopt)
        << "한도 글자 수의 한글 메시지는 받는다";
    EXPECT_EQ(ChatRequests::Validate(MakeHangulMessage(ChatRequests::MAX_MESSAGE_LENGTH + 1)), Protocol::CHAT_RESULT_INVALID_MESSAGE);
    EXPECT_EQ(ChatRequests::Validate(string(ChatRequests::MAX_MESSAGE_LENGTH + 1, 'a')), Protocol::CHAT_RESULT_INVALID_MESSAGE);
}
