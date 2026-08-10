#include "pch.h"
#include "TickIntervalTimer.h"

void TickIntervalTimer::Init(float intervalTime, IntervalFunc func, bool doOnce)
{
    Clear();
    _func = func;
    _intervalTime = intervalTime;
    _doOnce = doOnce;

    _state = TimerState::TIMER_STATE_RUNNING;
}

void TickIntervalTimer::Pause()
{
    _state = TimerState::TIMER_STATE_PAUSE;
}

void TickIntervalTimer::Tick(float deltaTime)
{
    if (_state != TimerState::TIMER_STATE_RUNNING)
        return;

    if (_doOnce && _repeatingCount > 0)
        return;

    _elapsedTime += deltaTime;

    // 넘었으면 트리거하고 다시 돌린다.
    if (_elapsedTime > _intervalTime)
    {
        _func();
        _elapsedTime -= _intervalTime;
        _repeatingCount++;
    }

}

void TickIntervalTimer::Clear()
{
    _doOnce = false;
    _elapsedTime = 0.f;
    _intervalTime = 0.f;
}

