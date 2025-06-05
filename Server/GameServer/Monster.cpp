#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
	objectInfo = new Protocol::ObjectInfo();
}

Monster::~Monster()
{
	delete objectInfo;
}