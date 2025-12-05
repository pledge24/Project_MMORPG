#pragma once
#include "Object.h"

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

    virtual void OnHit(ObjectRef attacker, Protocol::HitData& hitData) override;

protected:
    virtual void PostConstructionSetup() override;
    virtual void Tick(float deltaTime) override;

protected:
    bool idDead = false;
};

