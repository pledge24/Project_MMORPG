#include "pch.h"
#include "Object.h"

Object::Object()
{
	objectInfo = new Protocol::ObjectInfo();
	posInfo = new Protocol::PosInfo();
	objectInfo->set_allocated_pos_info(posInfo);

    tickGroupFuncs.resize(static_cast<int32>(ETickGroup::TG_COUNT));
}

Object::~Object()
{
	delete objectInfo;
}

void Object::TickThisGroup(ETickGroup tickGroup, float deltaTime)
{
    if (_isTickable == false)
        return;

    for (auto tickFunc : tickGroupFuncs[static_cast<int32>(tickGroup)])
    {
        tickFunc(deltaTime);
    }
}

void Object::PostConstructionSetup()
{
    weak_ptr<Object> weakSelf = shared_from_this();
    tickGroupFuncs[static_cast<int32>(ETickGroup::TG_PrePhysics)].push_back(
        [weakSelf](float deltaTime)
        {
            if (auto self = weakSelf.lock())
            {
                self->Tick(deltaTime);
            }
        }
    );
}

void Object::Tick(float deltaTime)
{
}

