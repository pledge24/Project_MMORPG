#pragma once

class TickTimer
{
public:
    TickTimer() = default;
    ~TickTimer() = default;

public:
    void Start(bool isRepeated = false);
    void Tick(float deltaSeconds);
    void Clear();

    void SetEndTime(float endTime);
    void SetEndTime(const vector<float>& endTimeList);

    int32 GetLastTriggered();

private:
    vector<float> _endTimeList;
    bool _repeated;
    float _elapsedTime = 0.f;
};

