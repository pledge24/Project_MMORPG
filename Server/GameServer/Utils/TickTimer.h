#pragma once
class TickTimer
{
public:
    TickTimer() = default;
    TickTimer(float time) : _endTime(time) {};

public:
    void Start();
    void Tick(float deltaTime);
    void Restart();
    void ClearTimer();

    void SetEndTime(float time) { _endTime = time; }
    void SetCheckPoint(float time, string name);

    bool IsCompleted();
    bool IsCheckPointReached(const string& name);
    bool IsRunning() { return _started; }

private:
    float _elapsedTime = 0.f;
    float _endTime = 0.f;
    bool _started = false;
    map<string, float> _checkPointMappings;
};

