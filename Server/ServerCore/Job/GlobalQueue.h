#pragma once

/*----------------
	GlobalQueue
-----------------*/

/**
 * 실행할 잡이 남았지만 지금 스레드가 맡지 못한 JobQueue를 모아 두는 대기열. 전역 객체 GGlobalQueue 하나만 있다.
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

