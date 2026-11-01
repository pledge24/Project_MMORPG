#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Data/ActionData.h"

/*--------------------------------------------------------------
    타격 기준 위치 테스트

    전사 콤보는 루트 모션으로 앞으로 나가며 휘두른다. 타격의 기준 위치를 시작 위치로 두면 앞으로 나간 만큼 공격 범위가
    캐릭터 뒤에 남는다. 기준 위치는 시작 위치에 루트 모션 곡선의 타격 시점 값을 시작 방향으로 돌려 더한 것이다.
    곡선은 1/표본 수 초 간격이고 마지막 표본만 섹션 끝이다. 표본 사이는 선형 보간한다.

    픽스처 결합도: 없음. 순수 함수만 쓴다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SAMPLE_RATE = 10;
    const vector3D START_FOOT{ 100.f, 200.f, 30.f };

    /** 0.1초마다 앞(+X)으로 10cm, 오른쪽(+Y)으로 1cm 가고 마지막 표본은 0.25초의 섹션 끝인 곡선. */
    ActionTiming MakeTiming(float hitTime)
    {
        ActionTiming timing;
        timing.length = 0.25f;
        timing.hitTimes = { hitTime };
        timing.rootMotion = { { 0.f, 0.f, 0.f }, { 10.f, 1.f, 0.f }, { 20.f, 2.f, 0.f }, { 25.f, 4.f, 2.f } };
        return timing;
    }

    void ExpectNear(const vector3D& actual, const vector3D& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, 0.001f);
        EXPECT_NEAR(actual.y, expected.y, 0.001f);
        EXPECT_NEAR(actual.z, expected.z, 0.001f);
    }
}

TEST(ActionHitPose, WithoutRootMotionHitsAtStartFoot)
{
    ActionTiming timing = MakeTiming(0.2f);
    timing.rootMotion.clear();

    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 30.f, timing, 0, SAMPLE_RATE);

    ExpectNear(pose.foot, START_FOOT);
    EXPECT_FLOAT_EQ(pose.yaw, 30.f) << "방향은 시작 방향이다";
}

TEST(ActionHitPose, AddsRootMotionAtHitTime)
{
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 0.f, MakeTiming(0.2f), 0, SAMPLE_RATE);

    ExpectNear(pose.foot, { 120.f, 202.f, 30.f });
}

TEST(ActionHitPose, InterpolatesBetweenSamples)
{
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 0.f, MakeTiming(0.15f), 0, SAMPLE_RATE);

    ExpectNear(pose.foot, { 115.f, 201.5f, 30.f });
}

TEST(ActionHitPose, LastSampleIsAtSectionEnd)
{
    // 마지막 표본은 0.3초가 아니라 섹션 끝 0.25초의 값이다. 0.225초는 0.2초 표본과 끝 표본의 가운데다.
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 0.f, MakeTiming(0.225f), 0, SAMPLE_RATE);

    ExpectNear(pose.foot, { 122.5f, 203.f, 31.f });
}

TEST(ActionHitPose, HitAfterSectionEndUsesLastSample)
{
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 0.f, MakeTiming(0.4f), 0, SAMPLE_RATE);

    ExpectNear(pose.foot, { 125.f, 204.f, 32.f });
}

TEST(ActionHitPose, RotatesRootMotionByStartYaw)
{
    // yaw는 +X에서 +Y 쪽으로 돈다. 90도면 캐릭터의 앞(+X)이 월드 +Y, 오른쪽(+Y)이 월드 -X다.
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 90.f, MakeTiming(0.2f), 0, SAMPLE_RATE);

    ExpectNear(pose.foot, { 98.f, 220.f, 30.f });
    EXPECT_FLOAT_EQ(pose.yaw, 90.f);
}

TEST(ActionHitPose, MissingSampleRateHitsAtStartFoot)
{
    const AttackerPose pose = ActionData::MakeHitPose(START_FOOT, 0.f, MakeTiming(0.2f), 0, 0);

    ExpectNear(pose.foot, START_FOOT);
}
