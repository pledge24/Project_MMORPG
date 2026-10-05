#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "JobTimer.h"

/*--------------
	JobQueue
---------------*/

/**
 * 넣은 잡을 순서대로, 한 번에 한 스레드만 실행하는 큐. Room이 상속해 룸 상태를 락 없이 직렬화한다.
 * 실행할 스레드가 따로 정해져 있지 않다. 비어 있던 큐에 잡을 넣은 스레드가 실행을 맡거나 GlobalQueue로 넘긴다.
 * shared_from_this를 쓰므로 반드시 shared_ptr로 만든다.
 */
class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	/** 잡을 만들어 Push한다. 큐가 비어 있으면 이 호출 안에서 바로 실행될 수 있다. */
	void DoAsync(CallbackType&& callback)
	{
		Push(make_shared<Job>(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::*memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	/** tickAfter(ms) 뒤에 이 큐로 들어가도록 JobTimer에 예약한다. 그때 큐가 소멸했으면 실행하지 않는다. */
	void DoTimer(uint64 tickAfter, CallbackType&& callback)
	{
		JobRef job = make_shared<Job>(std::move(callback));
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void DoTimer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobRef job = make_shared<Job>(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	/** 대기 중인 잡을 버린다. 잡 개수를 줄이지 않으므로 이후에 넣은 잡이 실행되지 않는다(TD-014). */
	void					ClearJobs() { _jobs.Clear(); }

public:
	/**
	 * 비어 있던 큐에 넣은 스레드가 실행을 맡는다. 그 스레드가 다른 큐를 실행 중이 아니고 pushOnly가 false면
	 * 바로 Execute하고, 아니면 GlobalQueue로 넘긴다.
	 */
	void					Push(JobRef job, bool pushOnly = false);
	/**
	 * 쌓인 잡을 모두 실행한다. 실행하는 동안 LCurrentJobQueue가 이 큐를 가리킨다.
	 * LEndTickCount를 넘기면 남은 잡을 GlobalQueue로 넘기고 리턴한다.
	 */
	void					Execute();

protected:
	LockQueue<JobRef>		_jobs;
	atomic<int32>			_jobCount = 0;
};

