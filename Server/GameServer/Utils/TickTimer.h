#pragma once
class TickTimer
{
public:
    TickTimer() = default;
    TickTimer(float time) : endTime(time) {};

public:
    void Start();
    void Tick(float deltaTime);
    void Restart();
    void ClearTimer();

    void SetEndTime(float time) { endTime = time; }
    void SetCheckPoint(float time, string name);

    bool IsCompleted();
    bool IsCheckPointReached(const string& name);
    bool IsRunning() { return started; }

private:
    float elapsedTime = 0.f;
    float endTime = 0.f;
    bool started = false;
    map<string, float> checkPointMappings;
};

