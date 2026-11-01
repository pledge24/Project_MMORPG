#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Action/ActionValidation.h"
#include "TestCreatureCombat.h"

/*--------------------------------------------------------------
    액션 요청 판정 테스트

    서버는 클라이언트가 보낸 액션 번호와 방향을 그대로 알렸다. 조작한 클라이언트는 콤보의 마지막 단계만 보내거나
    입력 창을 기다리지 않고 공격을 끊임없이 이어 붙일 수 있었다. 콤보는 직업의 첫 단계에서 시작해 NextAction을 따르고,
    직전 액션이 끝나기 전에는 그 액션의 콤보 입력 창이 열린 뒤에만 이어진다. 시점은 지연 허용치만큼 앞뒤로 너그럽다.

    픽스처 결합도: 실제 데이터와 같은 길이와 입력 창의 4단 콤보를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 COMBO_1 = 1001;
    constexpr int32 COMBO_2 = 1002;
    constexpr int32 COMBO_3 = 1003;
    constexpr int32 COMBO_4 = 1004;
    constexpr float TOLERANCE = ActionValidation::LATENCY_TOLERANCE;

    /** 실제 추출 값과 같은 길이와 입력 창이다. 2단계의 입력 창은 섹션 끝 직전에 닫힌다. */
    struct ComboStep
    {
        int32 actionId;
        optional<int32> nextAction;
        float length;
        optional<ComboWindow> window;
    };
    const ComboStep COMBO_STEPS[] = {
        { COMBO_1, COMBO_2, 1.8333f, ComboWindow{ 0.487f, 1.0667f } },
        { COMBO_2, COMBO_3, 1.3667f, ComboWindow{ 0.6675f, 1.3333f } },
        { COMBO_3, COMBO_4, 0.8f, ComboWindow{ 0.2976f, 0.7667f } },
        { COMBO_4, nullopt, 1.4333f, nullopt },
    };

    const ComboStep& Step(int32 actionId)
    {
        for (const ComboStep& step : COMBO_STEPS)
        {
            if (step.actionId == actionId)
                return step;
        }
        throw std::out_of_range("콤보에 없는 액션");
    }

    optional<ActionRejection> Request(int32 actionId, const optional<LastAction>& last)
    {
        return ActionValidation::ValidateCombo(actionId, COMBO_1, last);
    }

    Protocol::CombatResult ResultOf(const optional<ActionRejection>& rejection)
    {
        return rejection.has_value() ? rejection->result : Protocol::COMBAT_RESULT_SUCCESS;
    }
}

class ActionValidationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        for (const ComboStep& step : COMBO_STEPS)
        {
            DesignData::ActionRow action = TestCreatureCombat::MakeAction(step.actionId, TestCreatureCombat::MakeCircleHit(200.f));
            action.NextAction = step.nextAction;
            tables.actions[step.actionId] = action;

            ActionTiming timing = TestCreatureCombat::MakeTiming(0.1f);
            timing.length = step.length;
            timing.comboWindow = step.window;
            tables.actionTimings[step.actionId] = timing;
        }
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }
};

TEST_F(ActionValidationTest, FirstStepWithoutLastActionIsAccepted)
{
    EXPECT_EQ(Request(COMBO_1, nullopt), nullopt);
}

TEST_F(ActionValidationTest, LaterStepWithoutLastActionIsRejected)
{
    EXPECT_EQ(ResultOf(Request(COMBO_4, nullopt)), Protocol::COMBAT_RESULT_INVALID_ACTION) << "콤보의 마지막 단계만 골라 보낼 수 없다";
}

TEST_F(ActionValidationTest, FourStepsAtComboWindowsAreAllAccepted)
{
    // 클라이언트는 입력 창 안에서 누른 다음 단계를 창이 닫힐 때 시작한다. 창이 열리는 순간과 닫히는 순간 모두 받아들인다.
    for (const bool atWindowEnd : { false, true })
    {
        optional<LastAction> last;
        ASSERT_EQ(Request(COMBO_1, last), nullopt);
        last = LastAction{ COMBO_1, 0.f };

        for (const int32 next : { COMBO_2, COMBO_3, COMBO_4 })
        {
            const ComboWindow window = Step(last->actionId).window.value();
            last->elapsed = atWindowEnd ? window.end : window.start;

            EXPECT_EQ(Request(next, last), nullopt) << "액션 " << last->actionId << " 뒤 " << last->elapsed << "초의 " << next;
            last = LastAction{ next, 0.f };
        }
    }
}

TEST_F(ActionValidationTest, NextStepBeforeComboWindowIsRejected)
{
    const ComboWindow window = Step(COMBO_1).window.value();

    const auto rejection = Request(COMBO_2, LastAction{ COMBO_1, window.start - TOLERANCE - 0.01f });

    EXPECT_EQ(ResultOf(rejection), Protocol::COMBAT_RESULT_TOO_EARLY) << "입력 창을 기다리지 않고 다음 단계를 이어 붙일 수 없다";
}

TEST_F(ActionValidationTest, NextStepWithinToleranceBeforeComboWindowIsAccepted)
{
    const ComboWindow window = Step(COMBO_1).window.value();

    EXPECT_EQ(Request(COMBO_2, LastAction{ COMBO_1, window.start - TOLERANCE + 0.01f }), nullopt) << "지연의 흔들림만큼은 일찍 와도 받는다";
}

TEST_F(ActionValidationTest, SkippedStepIsRejected)
{
    const ComboWindow window = Step(COMBO_1).window.value();

    EXPECT_EQ(ResultOf(Request(COMBO_3, LastAction{ COMBO_1, window.start })), Protocol::COMBAT_RESULT_INVALID_ACTION);
}

TEST_F(ActionValidationTest, ActionOutsideComboIsRejected)
{
    EXPECT_EQ(ResultOf(Request(2001, nullopt)), Protocol::COMBAT_RESULT_INVALID_ACTION) << "직업의 콤보에 없는 액션(몬스터의 액션)";
    EXPECT_EQ(ResultOf(Request(2001, LastAction{ COMBO_1, 1.f })), Protocol::COMBAT_RESULT_INVALID_ACTION);
}

TEST_F(ActionValidationTest, NextStepSlightlyAfterLastActionEndedIsAccepted)
{
    // 2단계의 입력 창은 섹션 끝 0.03초 전에 닫힌다. 창이 닫힐 때 보낸 요청이 지연으로 섹션 끝 뒤에 도착해도 콤보가 이어져야 한다.
    const float length = Step(COMBO_2).length;

    EXPECT_EQ(Request(COMBO_3, LastAction{ COMBO_2, length + TOLERANCE - 0.01f }), nullopt);
}

TEST_F(ActionValidationTest, NextStepLongAfterLastActionEndedIsRejected)
{
    const float length = Step(COMBO_2).length;

    const auto rejection = Request(COMBO_3, LastAction{ COMBO_2, length + TOLERANCE + 0.01f });

    EXPECT_EQ(ResultOf(rejection), Protocol::COMBAT_RESULT_INVALID_ACTION) << "끝난 액션의 다음 단계로는 이어지지 않는다. 콤보는 첫 단계부터 다시 시작한다";
}

TEST_F(ActionValidationTest, FirstStepAfterLastActionEndedIsAccepted)
{
    for (const ComboStep& step : COMBO_STEPS)
        EXPECT_EQ(Request(COMBO_1, LastAction{ step.actionId, step.length }), nullopt) << "액션 " << step.actionId << "이 끝난 뒤";
}

TEST_F(ActionValidationTest, FirstStepDuringActionWithoutComboWindowIsRejected)
{
    const float length = Step(COMBO_4).length;

    const auto rejection = Request(COMBO_1, LastAction{ COMBO_4, length - TOLERANCE - 0.01f });

    EXPECT_EQ(ResultOf(rejection), Protocol::COMBAT_RESULT_TOO_EARLY) << "입력 창이 없는 마지막 단계는 끝날 때까지 새 콤보로 끊을 수 없다";
}

TEST_F(ActionValidationTest, FirstStepDuringActionBeforeComboWindowIsRejected)
{
    const ComboWindow window = Step(COMBO_1).window.value();

    const auto rejection = Request(COMBO_1, LastAction{ COMBO_1, window.start - TOLERANCE - 0.01f });

    EXPECT_EQ(ResultOf(rejection), Protocol::COMBAT_RESULT_TOO_EARLY) << "첫 단계를 다시 보내 타격 전에 끊고 재시작하는 반복을 막는다";
}

TEST(ActionYawValidation, YawInsideHalfTurnIsAccepted)
{
    for (const float yaw : { -180.f, -45.5f, 0.f, 179.9f, 180.f })
        EXPECT_EQ(ActionValidation::ValidateYaw(yaw), nullopt) << yaw;
}

TEST(ActionYawValidation, YawOutsideHalfTurnOrNotFiniteIsRejected)
{
    for (const float yaw : { -180.5f, 181.f, 720.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
    {
        const auto rejection = ActionValidation::ValidateYaw(yaw);
        ASSERT_TRUE(rejection.has_value()) << yaw;
        EXPECT_EQ(rejection->result, Protocol::COMBAT_RESULT_INVALID_YAW) << yaw;
    }
}
