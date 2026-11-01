#include "Core/pch.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Game/Combat/AttackArea.h"

/*--------------------------------------------------------------
    공격 범위 판정 테스트

    타격 하나의 공격 범위가 대상 캡슐에 닿는지와, 범위가 앞으로 닿는 거리를 판정한다.
    평면에서는 대상 원이 형상에 닿아야 하고(경계에 닿기만 해도 걸린다), 높이에서는 대상 캡슐의 구간이
    공격자 발밑 기준 높이 범위와 겹쳐야 한다.

    사례는 Data/AttackAreaCases.json에 있다. 클라이언트의 공격 범위 디버그 표시 테스트가 같은 파일을 읽어
    같은 답을 내는지 본다. 화면에 그린 범위가 곧 서버의 판정 범위여야 하기 때문이다.

    픽스처 결합도: 없음. 순수 함수와 사례 파일만 쓴다.
---------------------------------------------------------------*/

namespace
{
    const filesystem::path CASES_FILE = filesystem::path(__FILE__).parent_path() / "Data" / "AttackAreaCases.json";

    Json ReadCases()
    {
        ifstream file(CASES_FILE);
        if (file.is_open() == false)
            return Json();

        return Json::parse(file);
    }

    AttackShape ReadShape(const string& name)
    {
        if (name == "Circle")
            return AttackShape::Circle;
        if (name == "Sector")
            return AttackShape::Sector;
        if (name == "Rectangle")
            return AttackShape::Rectangle;

        ADD_FAILURE() << "사례 파일에 모르는 형상이 있다: " << name;
        return AttackShape::Circle;
    }

    AttackArea ReadArea(const Json& object)
    {
        AttackArea area;
        area.shape = ReadShape(object.at("Shape").get<string>());
        area.radius = object.at("Radius").get<float>();
        area.angle = object.at("Angle").get<float>();
        area.length = object.at("Length").get<float>();
        area.width = object.at("Width").get<float>();
        area.offsetX = object.at("OffsetX").get<float>();
        area.offsetY = object.at("OffsetY").get<float>();
        area.zMin = object.at("ZMin").get<float>();
        area.zMax = object.at("ZMax").get<float>();
        return area;
    }

    AttackerPose ReadAttacker(const Json& object)
    {
        AttackerPose attacker;
        attacker.foot = { object.at("X").get<float>(), object.at("Y").get<float>(), object.at("Z").get<float>() };
        attacker.yaw = object.at("Yaw").get<float>();
        return attacker;
    }

    TargetCapsule ReadTarget(const Json& object)
    {
        TargetCapsule target;
        target.foot = { object.at("X").get<float>(), object.at("Y").get<float>(), object.at("Z").get<float>() };
        target.radius = object.at("Radius").get<float>();
        target.halfHeight = object.at("HalfHeight").get<float>();
        return target;
    }
}

TEST(AttackArea, SharedOverlapCasesMatchExpected)
{
    const Json cases = ReadCases();
    ASSERT_TRUE(cases.contains("OverlapCases")) << "사례 파일을 읽지 못했다: " << CASES_FILE.string();
    // 사례가 하나도 없으면 아무것도 검사하지 않고 통과한다.
    ASSERT_FALSE(cases["OverlapCases"].empty());

    set<AttackShape> coveredShapes;
    for (const Json& testCase : cases["OverlapCases"])
    {
        SCOPED_TRACE(testCase.at("Name").get<string>());
        const AttackArea area = ReadArea(testCase.at("Area"));
        coveredShapes.insert(area.shape);

        EXPECT_EQ(AttackAreaMath::Overlaps(area, ReadAttacker(testCase.at("Attacker")), ReadTarget(testCase.at("Target"))),
            testCase.at("Expected").get<bool>());
    }

    EXPECT_EQ(coveredShapes.size(), 3u) << "형상마다 사례가 있어야 한다";
}

TEST(AttackArea, SharedCasesStateServerTolerance)
{
    // 경계 사례의 답은 여유에 달려 있다. 클라이언트는 사례 파일의 여유를 읽어 같은 답을 낸다.
    const Json cases = ReadCases();
    ASSERT_TRUE(cases.contains("Tolerance")) << "사례 파일을 읽지 못했다: " << CASES_FILE.string();

    EXPECT_FLOAT_EQ(cases["Tolerance"].get<float>(), AttackAreaMath::OVERLAP_TOLERANCE);
}

TEST(AttackArea, SharedFrontReachCasesMatchExpected)
{
    const Json cases = ReadCases();
    ASSERT_TRUE(cases.contains("FrontReachCases")) << "사례 파일을 읽지 못했다: " << CASES_FILE.string();
    ASSERT_FALSE(cases["FrontReachCases"].empty());

    for (const Json& testCase : cases["FrontReachCases"])
    {
        SCOPED_TRACE(testCase.at("Name").get<string>());

        EXPECT_FLOAT_EQ(AttackAreaMath::GetFrontReach(ReadArea(testCase.at("Area"))), testCase.at("Expected").get<float>());
    }
}

// 룸은 공격자 발밑에서 이 거리 안의 셀에서 후보를 모은다. 닿는 대상이 그 밖에 있으면 맞아야 할 대상이 빠진다.
TEST(AttackArea, PlaneReachCoversEveryOverlappingTarget)
{
    const Json cases = ReadCases();
    ASSERT_TRUE(cases.contains("OverlapCases")) << "사례 파일을 읽지 못했다: " << CASES_FILE.string();

    for (const Json& testCase : cases["OverlapCases"])
    {
        if (testCase.at("Expected").get<bool>() == false)
            continue;

        SCOPED_TRACE(testCase.at("Name").get<string>());
        const AttackArea area = ReadArea(testCase.at("Area"));
        const AttackerPose attacker = ReadAttacker(testCase.at("Attacker"));
        const TargetCapsule target = ReadTarget(testCase.at("Target"));

        const float distance = hypot(target.foot.x - attacker.foot.x, target.foot.y - attacker.foot.y);
        EXPECT_LE(distance, AttackAreaMath::GetPlaneReach(area) + target.radius + AttackAreaMath::OVERLAP_TOLERANCE);
    }
}

TEST(AttackArea, PlaneReachOfOffsetRectangleReachesFarCorner)
{
    AttackArea area;
    area.shape = AttackShape::Rectangle;
    area.length = 300.f;
    area.width = 80.f;
    area.offsetX = 30.f;
    area.offsetY = 40.f;

    // 기준점까지 50, 기준점에서 먼 모서리까지 hypot(300, 40)이다.
    EXPECT_FLOAT_EQ(AttackAreaMath::GetPlaneReach(area), 50.f + hypot(300.f, 40.f));
}

TEST(AttackArea, TouchingTargetIsHitAfterRotation)
{
    // 공격자를 돌린 계산은 부동소수 오차를 낸다. 경계에 닿은 대상이 방향에 따라 걸렸다 빠졌다 하면 안 된다.
    AttackArea area;
    area.shape = AttackShape::Rectangle;
    area.length = 300.f;
    area.width = 100.f;
    area.zMax = 200.f;

    TargetCapsule target;
    target.radius = 30.f;
    target.halfHeight = 90.f;

    for (float yaw = -180.f; yaw < 180.f; yaw += 15.f)
    {
        SCOPED_TRACE(yaw);
        const float radians = yaw * numbers::pi_v<float> / 180.f;
        // 앞쪽 변에 닿는 자리. 공격자 기준 (330, 0)이다.
        target.foot = { 330.f * cos(radians), 330.f * sin(radians), 0.f };

        EXPECT_TRUE(AttackAreaMath::Overlaps(area, AttackerPose{ {}, yaw }, target));
    }
}
