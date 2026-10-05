#pragma once

/*------------------
	DBQueue
-------------------*/

/**
 * DB 스레드 하나가 소비하는 잡 큐. JobQueue와 달리 넣은 스레드가 실행하지 않는다.
 * 전용 DB 스레드가 WaitForSingleJob으로 블로킹 대기하므로, DB 호출이 IOCP 워커를 막지 않는다.
 */
class DBQueue
{
public:
    DBQueue(int32 dbQueueId);
    ~DBQueue();

    /** 여러 스레드에서 불러도 된다. 기다리는 DB 스레드 하나를 깨운다. */
    void                        Push(JobRef&& job);
    /** 잡이 들어올 때까지 블로킹한다. 멈춤 상태이고 비어 있으면 nullptr를 돌려준다. */
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

