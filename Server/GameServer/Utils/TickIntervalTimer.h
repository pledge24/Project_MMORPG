#pragma once

enum class TimerState
{
    TIMER_STATE_NONE,
    TIMER_STATE_RUNNING,
    TIMER_STATE_PAUSE
};

using IntervalFunc = function<void()>;

/**
 * 호출자가 넘기는 Tick의 경과 시간을 누적해 intervalTime마다 func를 부르는 타이머.
 * 스스로 시간을 재지 않는다. 시간 단위는 Init과 Tick에 같은 단위를 넘기는 호출자가 정한다.
 * 락이 없으므로 소유자와 같은 스레드에서만 쓴다. 지금은 쓰는 곳이 없다.
 */
class TickIntervalTimer
{
public:
    TickIntervalTimer() = default;
    ~TickIntervalTimer() = default;

public:
    /** doOnce면 한 번 부른 뒤로 멈춘다. 부르면 바로 RUNNING 상태가 된다. */
    void Init(float intervalTime, IntervalFunc func, bool doOnce = false);
    /** 다시 돌리려면 Init을 다시 부른다. 재개 함수는 없다. */
    void Pause();
    /** RUNNING일 때만 누적한다. 한 번에 여러 주기가 지나도 func는 한 번만 부른다. */
    void Tick(float deltaTime);
    /** 누적 시간과 주기만 지운다. 상태와 실행 횟수는 그대로다. */
    void Clear();

    bool IsStarted() { return _state != TimerState::TIMER_STATE_NONE; }
    bool IsRunning() { return _state == TimerState::TIMER_STATE_RUNNING; }
    bool IsPausing() { return _state == TimerState::TIMER_STATE_PAUSE; }

private:
    IntervalFunc _func;
    TimerState _state = TimerState::TIMER_STATE_NONE;
    float _elapsedTime = 0.f;
    float _intervalTime = 0.f;  // 0이면 경과 시간이 0보다 큰 틱마다 발동.
    bool _doOnce = false;
    int32 _repeatingCount = 0;
};

