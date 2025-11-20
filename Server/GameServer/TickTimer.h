#pragma once

class TickTimer
{
public:
    TickTimer() = default;
    ~TickTimer() = default;

public:
    void Start(bool isRepeated = false);
    void Tick(float deltaTime);
    void Clear();

    void SetEndTime(float endTime);
    void SetEndTime(const vector<float>& endTimeList);

    int64 GetLastTriggered() const;
    bool IsStarted() { return _started; }

private:
    vector<float> _endTimeList;
    bool _repeated;
    float _elapsedTime = 0.f;
    bool _started = false;
};

