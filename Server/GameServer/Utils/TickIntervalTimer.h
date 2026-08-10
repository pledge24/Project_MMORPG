#pragma once

enum class TimerState
{
    TIMER_STATE_NONE,
    TIMER_STATE_RUNNING,
    TIMER_STATE_PAUSE
};

using IntervalFunc = function<void()>;

class TickIntervalTimer
{
public:
    TickIntervalTimer() = default;
    ~TickIntervalTimer() = default;

public:
    void Init(float intervalTime, IntervalFunc func, bool doOnce = false);
    void Pause();
    void Tick(float deltaTime);
    void Clear();

    bool IsStarted() { return _state != TimerState::TIMER_STATE_NONE; }
    bool IsRunning() { return _state == TimerState::TIMER_STATE_RUNNING; }
    bool IsPausing() { return _state == TimerState::TIMER_STATE_PAUSE; }

private:
    IntervalFunc _func;
    TimerState _state = TimerState::TIMER_STATE_NONE;
    float _elapsedTime = 0.f;
    float _intervalTime = 0.f;  // 0이면 매 틱마다 발동.
    bool _doOnce = false;
    int32 _repeatingCount = 0;
};

