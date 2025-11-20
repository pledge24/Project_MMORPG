#pragma once
#include "Object.h"

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

protected:
    virtual void Tick(float deltaTime) override;

};

