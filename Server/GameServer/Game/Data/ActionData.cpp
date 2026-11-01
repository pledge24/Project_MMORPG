#include "Core/pch.h"
#include "Game/Data/ActionData.h"

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
