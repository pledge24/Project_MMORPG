#include "pch.h"
#include "TickTimer.h"

void TickTimer::Start(bool isRepeated)
{
    _repeated = isRepeated;
    _elapsedTime = 0.f;
}

void TickTimer::Tick(float deltaSeconds)
{
    if (_endTimeList.empty())
        return;

    _elapsedTime += deltaSeconds;
}

void TickTimer::Clear()
{
    _endTimeList.clear();
}

void TickTimer::SetEndTime(float endTime)
{
    Clear();
    _endTimeList.push_back(endTime);
}

void TickTimer::SetEndTime(const vector<float>& endTimeList)
{
    Clear();
    std::sort(endTimeList.begin(), endTimeList.end());
    _endTimeList = endTimeList;
}

int32 TickTimer::GetLastTriggered()
{
    float timerCycle = _endTimeList.back();
    float timeline = _repeated ? fmod(_elapsedTime, timerCycle) : _elapsedTime;

    int32 lastEndTimeIdx = std::lower_bound(_endTimeList.begin(), _endTimeList.end(), timeline) - _endTimeList.begin();
    return lastEndTimeIdx;
}
