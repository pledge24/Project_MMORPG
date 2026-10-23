#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/WorldListing.h"

/*--------------------------------------------------------------
    월드 혼잡도 테스트

    게임 서버는 연결된 계정 수를 정원에 비춰 세 단계로 나눠 월드 항목에 쓴다. 정원의 50% 미만이 원활,
    80% 미만이 보통, 그 이상이 혼잡이다. 인증 서버는 혼잡도 값의 이름만 읽으므로 이름도 계약이다.

    픽스처 결합도: 없음. 순수 함수만 부른다. Redis에 쓰는 일은 덮지 않는다.
---------------------------------------------------------------*/

TEST(WorldListing, CongestionStepsAtHalfAndEightyPercentOfCapacity)
{
    EXPECT_EQ(WorldListing::ComputeCongestion(0, 100), WorldCongestion::Smooth);
    EXPECT_EQ(WorldListing::ComputeCongestion(49, 100), WorldCongestion::Smooth);
    EXPECT_EQ(WorldListing::ComputeCongestion(50, 100), WorldCongestion::Normal);
    EXPECT_EQ(WorldListing::ComputeCongestion(79, 100), WorldCongestion::Normal);
    EXPECT_EQ(WorldListing::ComputeCongestion(80, 100), WorldCongestion::Crowded);
    EXPECT_EQ(WorldListing::ComputeCongestion(100, 100), WorldCongestion::Crowded);
}

// 정원은 접속을 막지 않으므로 연결 계정이 정원을 넘을 수 있다.
TEST(WorldListing, OverCapacityIsCrowded)
{
    EXPECT_EQ(WorldListing::ComputeCongestion(150, 100), WorldCongestion::Crowded);
}

// 비율이 나눠떨어지지 않는 정원에서도 경계를 넘기 전까지는 아래 단계다. 3의 50%는 1.5, 80%는 2.4다.
TEST(WorldListing, BoundaryWithIndivisibleCapacity)
{
    EXPECT_EQ(WorldListing::ComputeCongestion(1, 3), WorldCongestion::Smooth);
    EXPECT_EQ(WorldListing::ComputeCongestion(2, 3), WorldCongestion::Normal);
    EXPECT_EQ(WorldListing::ComputeCongestion(3, 3), WorldCongestion::Crowded);
}

TEST(WorldListing, LargeCountDoesNotOverflow)
{
    EXPECT_EQ(WorldListing::ComputeCongestion(INT32_MAX, INT32_MAX), WorldCongestion::Crowded);
    EXPECT_EQ(WorldListing::ComputeCongestion(INT32_MAX / 2 - 1, INT32_MAX), WorldCongestion::Smooth);
}

// 인증 서버의 common/worldDirectory.ts가 이 이름으로 읽는다.
TEST(WorldListing, ContractNames)
{
    EXPECT_STREQ(WorldListing::ToContractName(WorldCongestion::Smooth), "SMOOTH");
    EXPECT_STREQ(WorldListing::ToContractName(WorldCongestion::Normal), "NORMAL");
    EXPECT_STREQ(WorldListing::ToContractName(WorldCongestion::Crowded), "CROWDED");
}
