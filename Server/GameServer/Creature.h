#pragma once
#include "Object.h"

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

protected:
    virtual void PostConstructionSetup() override;
    virtual void Tick(float deltaTime) override;

};

