#include "pch.h"
#include "DBQueue.h"

DBQueue::DBQueue()
{
}

DBQueue::~DBQueue()
{
}

void DBQueue::Push(JobRef job)
{
    {
        LockGuard lock(mtx);
        if (!stopFlag)
            jobs.push(std::move(job));
    }
    cv.notify_one();
}

JobRef DBQueue::WaitForSingleJob()
{
    UniqueLock lock(mtx);
    cv.wait(lock, [this]()
        {
            return stopFlag || !jobs.empty();
        });

    if (stopFlag && jobs.empty())
        return nullptr;

    JobRef job = std::move(jobs.front());
    jobs.pop();

    return job;
}
