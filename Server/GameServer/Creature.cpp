#include "pch.h"
#include "Creature.h"

Creature::Creature()
{
}

Creature::~Creature()
{

}

void Creature::OnHit(ObjectRef attacker, Protocol::HitData& hitData)
{

}

void Creature::PostConstructionSetup()
{
    Object::PostConstructionSetup();

}

void Creature::Tick(float deltaTime)
{
    Object::Tick(deltaTime);
}
