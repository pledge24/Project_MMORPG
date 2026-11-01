#include "Core/pch.h"
#include "Game/Data/ActionData.h"

namespace
{
    constexpr float DEGREES_TO_RADIANS = numbers::pi_v<float> / 180.f;

    // time초의 누적 루트 모션(캐릭터 로컬). 표본은 1/sampleRate초 간격이고 마지막 표본만 섹션 끝이다.
    // 마지막 표본까지 같은 간격으로 보면 끝 근처의 타격이 실제보다 덜 나간 곳을 맞힌다.
    vector3D SampleRootMotion(const ActionTiming& timing, float time, int32 sampleRate)
    {
        const vector<vector3D>& samples = timing.rootMotion;
        if (samples.empty() || sampleRate <= 0)
            return vector3D();

        const int32 lastIndex = static_cast<int32>(samples.size()) - 1;
        auto sampleTime = [&](int32 index)
            {
                return index == lastIndex ? timing.length : static_cast<float>(index) / static_cast<float>(sampleRate);
            };

        const int32 index = (std::max)(0, static_cast<int32>(floor(time * static_cast<float>(sampleRate))));
        if (index >= lastIndex || time >= timing.length)
            return samples[lastIndex];

        const float from = sampleTime(index);
        const float to = sampleTime(index + 1);
        const float alpha = to > from ? std::clamp((time - from) / (to - from), 0.f, 1.f) : 1.f;
        const vector3D& a = samples[index];
        const vector3D& b = samples[index + 1];
        return vector3D(a.x + (b.x - a.x) * alpha, a.y + (b.y - a.y) * alpha, a.z + (b.z - a.z) * alpha);
    }
}

AttackArea ActionData::ToAttackArea(const DesignData::ActionHitRow& hit)
{
    AttackArea area;
    {
        switch (hit.Shape)
        {
        case DesignData::AttackAreaShape::SECTOR:
            area.shape = AttackShape::Sector;
            break;
        case DesignData::AttackAreaShape::RECTANGLE:
            area.shape = AttackShape::Rectangle;
            break;
        default:
            area.shape = AttackShape::Circle;
            break;
        }

        // 형상이 쓰지 않는 값은 0이다(DataBuilder가 검사한다). 그대로 옮긴다.
        area.radius = hit.Radius;
        area.angle = hit.Angle;
        area.length = hit.Length;
        area.width = hit.Width;
        area.offsetX = hit.OffsetX;
        area.offsetY = hit.OffsetY;
        area.zMin = hit.ZMin;
        area.zMax = hit.ZMax;
    }
    return area;
}

DamageType ActionData::ToDamageType(DesignData::DamageType damageType)
{
    return damageType == DesignData::DamageType::MAGICAL ? DamageType::Magical : DamageType::Physical;
}

float ActionData::GetFrontReach(const DesignData::ActionRow& action)
{
    float reach = 0.f;
    for (const DesignData::ActionHitRow& hit : action.Hits)
        reach = (std::max)(reach, AttackAreaMath::GetFrontReach(ToAttackArea(hit)));
    return reach;
}

AttackerPose ActionData::MakeHitPose(const vector3D& startFoot, float yaw, const ActionTiming& timing, int32 hitIndex, int32 sampleRate)
{
    // 루트 모션은 캐릭터 기준(앞 +x, 오른쪽 +y)이다. 시작 방향으로 돌려 월드에 더한다. yaw는 +x에서 +y 쪽으로 돈다.
    const vector3D local = SampleRootMotion(timing, timing.hitTimes[hitIndex], sampleRate);
    const float yawRadians = yaw * DEGREES_TO_RADIANS;
    const float c = cos(yawRadians);
    const float s = sin(yawRadians);

    AttackerPose pose;
    {
        pose.foot = vector3D(startFoot.x + local.x * c - local.y * s, startFoot.y + local.x * s + local.y * c, startFoot.z + local.z);
        pose.yaw = yaw;
    }
    return pose;
}
