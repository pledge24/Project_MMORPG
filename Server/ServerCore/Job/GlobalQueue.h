#pragma once

/*----------------
	GlobalQueue
-----------------*/

/**
 * 전역으로 "딱 하나" 존재하는 LockQueue.
 * 처리할 Job이 남아있지만 스레드 배정을 받지 못한 JobQueue들을 모아두는 대기열 역할을 한다.
 * 워커 스레드가 ThreadManager::DoGlobalQueueWork에서 꺼내 실행한다.
 */
class GlobalQueue
{
public:
	GlobalQueue();
	~GlobalQueue();

	void					Push(JobQueueRef jobQueue);
	/** 비어 있으면 nullptr를 돌려준다. */
	JobQueueRef				Pop();

private:
	LockQueue<JobQueueRef> _jobQueues;
};

