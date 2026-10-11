#pragma once
#include "ServerCore/Job/Job.h"
#include "ServerCore/Utils/LockQueue.h"
#include "ServerCore/Job/JobTimer.h"

/*----------------
      JobQueue
-----------------*/

/**
 * Job을 저장하는 LockQueue 래퍼 클래스. 
 * producer : consumer = N : 1로 운영된다.
 * 실행할 스레드가 따로 정해져 있지 않다. 비어 있던 큐에 Job을 넣은 스레드가 실행을 맡거나 GlobalQueue로 넘긴다.
 */
class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	/** Job객체를 "람다" 버전으로 만들어 Push한다. 큐가 비어 있으면 이 호출 안에서 바로 실행될 수 있다. */
	void DoAsync(CallbackType&& callback)
	{
		Push(make_shared<Job>(std::move(callback)));
	}

    /** Job객체를 "멤버 함수" 버전으로 만들어 Push한다. 큐가 비어 있으면 이 호출 안에서 바로 실행될 수 있다. */
	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::*memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	/** 
	 * Job객체를 "람다" 버전으로 만들어 tickAfter(ms) 뒤에 이 큐로 들어가도록 전역 JobTimer에 예약한다. 
	 * 예약 시간에 도달한 시점에 큐가 소멸했으면 실행하지 않는다. 
	 */
	void DoTimer(uint64 tickAfter, CallbackType&& callback)
	{
		JobRef job = make_shared<Job>(std::move(callback));
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

    /** 
     * Job객체를 "멤버 함수" 버전으로 만들어 tickAfter(ms) 뒤에 이 큐로 들어가도록 전역 JobTimer에 예약한다. 
     * 예약 시간에 도달한 시점에 큐가 소멸했으면 실행하지 않는다. 
     */
	template<typename T, typename Ret, typename... Args>
	void DoTimer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobRef job = make_shared<Job>(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	// 대기 중인 잡을 버리는 함수를 두지 않는다. 잡 개수만 맞춰 줄이면, 실행을 맡은 스레드가 남아 있는 사이에 다음 Push가
	// 실행을 또 맡아 한 큐를 두 스레드가 실행한다(TD-014).

public:
	/**
	 * Job을 Queue에 밀어넣는다.
	 * 만약 Job을 Execute할 수 있는 상황이라면(pushOnly가 false이고, 해당 JobQueue를 Execute하는 스레드가 없음), Execute까지 맡는다.
	 * 아니면 GlobalQueue로 넘긴다.
	 */
	void					Push(JobRef job, bool pushOnly = false);
	/**
	 * 쌓인 잡을 모두 실행한다. 실행하는 동안 LCurrentJobQueue가 이 큐를 가리킨다.
	 * 일감이 특정 스레드에 몰리는 것을 방지하기 위해 
	 * 일정 시간(LEndTickCount)이상 작업하면 해당 JobQueueRef를 GlobalQueue에 넘기고 빠져나온다.
	 */
	void					Execute();

protected:
	LockQueue<JobRef>		_jobs;
	atomic<int32>			_jobCount = 0;
};

