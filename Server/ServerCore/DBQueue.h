#pragma once
class DBQueue
{
public:
    DBQueue();
    ~DBQueue();

    void Push(JobRef job);
    JobRef WaitForSingleJob();
    bool isStop() { return stopFlag == false; }

private:
    queue<JobRef> jobs;
    Mutex mtx;
    CondVar cv;
    Atomic<bool> stopFlag = false;
};

