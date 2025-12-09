#include "pch.h"
#include "Creature.h"
#include "Player.h"
#include "Room.h"

Creature::Creature()
{
    statInfo = new Protocol::StatInfo();
}

Creature::~Creature()
{
    delete statInfo;
}

void Creature::OnHit(ObjectRef attacker, Protocol::HitData& hitData)
{
    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    // TEMP: Hit 발생시 Hp만 깎도록 설정
    int64 damage = hitData.damage();
    int64 hp = GetStatValue(Protocol::STAT_TYPE_HP);
    int64 updatedHp = hp - damage;

    SetStatValue(Protocol::STAT_TYPE_HP, max(0, updatedHp));

    // Send Hit Packet
    {
        Protocol::S_HIT hitPkt;
        {
            hitPkt.mutable_hit_data()->CopyFrom(hitData);
            Protocol::Stat* stat = hitPkt.add_updated_stat();
            {
                stat->set_type(Protocol::STAT_TYPE_HP);
                stat->set_value(updatedHp);
            }
        }

        if (objectInfo->object_type() == Protocol::OBJECT_TYPE_PLAYER)
        {
            PlayerRef player = static_pointer_cast<Player>(shared_from_this());
            if (auto ownerSession = player->session.lock())
            {
                SEND_PACKET_USING_THIS_SESSION(ownerSession, hitPkt);
            }
        }
        else if(objectInfo->object_type() == Protocol::OBJECT_TYPE_MONSTER)
        {
            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(hitPkt);
            ownerRoom->Broadcast(sendBuffer);
        }
    }

    if (updatedHp <= 0)
    {
        OnDie(attacker);
    }
}

void Creature::OnDie(ObjectRef attacker)
{
    isDead = true;

    auto ownerRoom = room.load().lock();
    if (ownerRoom == nullptr)
        return;

    int64 objectId = objectInfo->object_id();

    Protocol::S_DIE DiePkt;
    {
        DiePkt.set_object_id(objectId);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(DiePkt);
        ownerRoom->Broadcast(sendBuffer);
    }
}

void Creature::SetStatValue(Protocol::StatType statType, const int64& value)
{
    auto* statMappings = statInfo->mutable_info();
    (*statMappings)[(int32)statType] = value;
}

int64 Creature::GetStatValue(Protocol::StatType statType)
{
    auto* statMappings = statInfo->mutable_info();
    return statMappings->at((int32)statType);
}

Protocol::Stat Creature::GetStat(Protocol::StatType statType)
{
    int64 value = GetStatValue(statType);
    Protocol::Stat stat;
    {
        stat.set_type(statType);
        stat.set_value(value);
    }

    return stat;
}

void Creature::PostConstructionSetup()
{
    Object::PostConstructionSetup();

}

void Creature::Tick(float deltaTime)
{
    Object::Tick(deltaTime);
}
