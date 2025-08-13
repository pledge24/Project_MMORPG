#pragma once
class DBQueue
{
public:
    DBQueue(int32 dbQueueId);
    ~DBQueue();

    void                        Push(JobRef&& job);
    JobRef                      WaitForSingleJob();

    bool                        IsStop() { return stopFlag == true; }
    int32                       GetId() { return _dbQueueId; }

private:
    queue<JobRef> jobs;
    Mutex mtx;
    CondVar cv;
    Atomic<bool> stopFlag = false;
    int32 _dbQueueId;
};

