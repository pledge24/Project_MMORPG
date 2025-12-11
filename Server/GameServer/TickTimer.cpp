#include "pch.h"
#include "TickTimer.h"

void TickTimer::Start()
{
    started = true;
}

void TickTimer::Tick(float deltaTime)
{
    if (started == false)
        return;

    elapsedTime += deltaTime;
}

void TickTimer::Restart()
{
    elapsedTime = 0.f;
}

void TickTimer::ClearTimer()
{
    elapsedTime = 0.f;
    endTime = 0.f;
    started = false;
    checkPointMappings.clear();
}

void TickTimer::SetCheckPoint(float time, string name)
{
    checkPointMappings.insert(make_pair(name, time));
}

bool TickTimer::IsCompleted()
{
    return elapsedTime >= endTime; 
}

bool TickTimer::IsCheckPointReached(const string& name)
{
    return elapsedTime >= checkPointMappings[name];
}
