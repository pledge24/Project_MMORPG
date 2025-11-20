#include "pch.h"
#include "TickTimer.h"

void TickTimer::Start(bool isRepeated)
{
    _repeated = isRepeated;
    _elapsedTime = 0.f;
    _started = true;
}

void TickTimer::Tick(float deltaTime)
{
    if (_endTimeList.empty())
        return;

    _elapsedTime += deltaTime;
}

void TickTimer::Clear()
{
    _endTimeList.clear();
    _repeated = false;
    _elapsedTime = 0.f;
    _started = false;
}

void TickTimer::SetEndTime(float endTime)
{
    Clear();
    _endTimeList.push_back(endTime);
}

void TickTimer::SetEndTime(const vector<float>& endTimeList)
{
    Clear();
    _endTimeList = endTimeList;
    std::sort(_endTimeList.begin(), _endTimeList.end());
}

int64 TickTimer::GetLastTriggered() const
{
    float timerCycle = _endTimeList.back();
    float timeline = _repeated ? fmod(_elapsedTime, timerCycle) : _elapsedTime;

    int64 lastEndTimeIdx = std::lower_bound(_endTimeList.begin(), _endTimeList.end(), timeline) - _endTimeList.begin();
    return lastEndTimeIdx;
}
