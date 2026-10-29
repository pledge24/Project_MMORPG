#pragma once
#include "Game/Entities/Entity.h"

/** NPC의 스폰 매개변수. */
struct NpcSpawnParams : public Entity::SpawnParams
{
    int32 npcId = 0;
    /** 높이는 NPC 마커의 바닥 높이다. */
    Protocol::PosInfo spawnPos;
};

/**
 * 플레이어가 조준해 상호작용하는 엔티티.
 *
 * 레벨의 NPC 마커 자리에 서서 움직이지 않고 싸우지 않는다. NPC 표가 정한 상점을 연다.
 */
class Npc : public Entity
{
public:
    using SpawnParams = NpcSpawnParams;

    Npc();
    virtual ~Npc();

protected:
    friend class EntityFactory;
    /** NPC 표에 없는 npcId면 false. */
    bool Init(const SpawnParams& params);

    //~ 상점
public:
    /** 이 NPC의 상점이 templateId 아이템을 팔면 true. */
    bool Sells(int32 templateId) const;

    /**
     * pos에서 이 NPC와 거래할 수 있으면 true. 평면 거리가 상호작용 사거리에 여유를 더한 거리 안이다.
     * 여유는 클라이언트의 위치가 서버보다 앞선 만큼을 받아 준다.
     */
    bool IsWithinReach(const Protocol::PosInfo& pos) const;

private:
    /** 사거리에 더하는 여유(cm). 이동 패킷 주기 동안 달릴 수 있는 거리보다 넉넉하다. */
    static constexpr float REACH_TOLERANCE = 100.f;

    /** NPC 표의 행 사본. */
    DesignData::NpcRow _template;

    //~ Entity 정보
public:
    int32 GetNpcId() const { return _template.Id; }
};
