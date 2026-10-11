#pragma once

/** 
 * Job 예약 정보가 담긴 구조체.
 * JobQueue가 먼저 소멸될 수 있도록 weak_ptr로 든다. 
 */
struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{

	}

	weak_ptr<JobQueue>	owner;
	JobRef				job;
};

/** 예약된 Job 구조체 */
struct TimerItem
{
    // 예약된 시간(executeTick)이 클수록(늦을수록) 우선순위가 낮아지도록 설정해야 
    // 시간이 얼마 안 남은 Job이 앞으로 온다.
	bool operator<(const TimerItem& other) const
	{
		return executeTick > other.executeTick;
	}

	uint64 executeTick = 0;
	JobData* jobData = nullptr;
};

/*--------------
	JobTimer
---------------*/

/**
 * JobQueue::DoTimer로 예약한 Job을 시각순으로 들고 있는 타이머 컨테이너.
 * 외부에 의해 Distribute 된다. 자체적으로 Distribute를 호출하지는 않는다.
 * 현재 전역 객체인 GJobTimer 하나만 있고, 워커 스레드가 루프마다 Distribute를 부른다.
 */
class JobTimer
{
public:
	/** tickAfter(ms) 뒤에 JobQueue(owner)에 job을 넣어주도록 예약을 건다. 여러 스레드에서 불러도 된다. */
	void			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
	/**
	 * now 기준으로 예약 시간이 된 job들을 각 owner에 넣는다. 
	 * 한 번에 한 스레드만 실행하고, 나머지는 바로 리턴한다.
	 * owner가 이미 소멸했으면 그 job은 버린다. 잡을 실행하지는 않는다. 비어 있던 큐는 GlobalQueue로 넘어가 워커가 실행한다.
	 */
	void			Distribute(uint64 now);
	/** 남은 예약을 모두 버린다. */
	void			Clear();

private:
	MAKE_LOCK;
	priority_queue<TimerItem>	_items;
	atomic<bool>				_distributing = false;
};

