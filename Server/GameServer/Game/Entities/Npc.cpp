#include "Core/pch.h"
#include "Game/Entities/Npc.h"

Npc::Npc()
{
    _isPlayer = false;
}

Npc::~Npc()
{
}

bool Npc::Init(const SpawnParams& params)
{
    if (Entity::Init(params) == false)
        return false;

    const DesignData::NpcRow* npcRow = Gamedata::FindNpc(params.npcId);
    if (npcRow == nullptr)
    {
        GLogger->Warning("NPC 표에 없는 번호 {}로 NPC를 만들 수 없다", params.npcId);
        return false;
    }

    _template = *npcRow;
    SetPosInfo(params.spawnPos);
    _entityInfo->mutable_npc_info()->set_npc_id(_template.Id);

    return true;
}

bool Npc::Sells(int32 templateId) const
{
    // 기동할 때 NPC 표의 상점이 상점 표에 있는지 대조했다. 없으면 표를 바꿔 끼운 테스트다.
    const DesignData::ShopRow* shop = Gamedata::FindShop(_template.ShopId);
    if (shop == nullptr)
        return false;

    return ranges::find(shop->Items, templateId) != shop->Items.end();
}

bool Npc::IsWithinReach(const Protocol::PosInfo& pos) const
{
    // 높이는 보지 않는다. NPC의 z는 바닥이고 플레이어의 z는 캡슐 중심이다.
    const float dx = pos.pos().x() - _posInfo->pos().x();
    const float dy = pos.pos().y() - _posInfo->pos().y();
    const float reach = Gamedata::GetInteractionRange() + REACH_TOLERANCE;
    return dx * dx + dy * dy <= reach * reach;
}
