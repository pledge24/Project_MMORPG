#include "pch.h"
#include "TickTimer.h"

void TickTimer::Start()
{
    _started = true;
}

void TickTimer::Tick(float deltaTime)
{
    if (_started == false)
        return;

    _elapsedTime += deltaTime;
}

void TickTimer::Restart()
{
    _elapsedTime = 0.f;
}

void TickTimer::ClearTimer()
{
    _elapsedTime = 0.f;
    _endTime = 0.f;
    _started = false;
    _checkPointMappings.clear();
}

void TickTimer::SetCheckPoint(float time, string name)
{
    _checkPointMappings.insert(make_pair(name, time));
}

bool TickTimer::IsCompleted()
{
    return _elapsedTime >= _endTime; 
}

bool TickTimer::IsCheckPointReached(const string& name)
{
    return _elapsedTime >= _checkPointMappings[name];
}
