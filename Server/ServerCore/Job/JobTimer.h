#pragma once

// 예약된 잡과 그 잡을 넣을 큐. 큐가 먼저 소멸해도 되도록 weak_ptr로 든다.
struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{

	}

	weak_ptr<JobQueue>	owner;
	JobRef				job;
};

// 실행 시각(GetTickCount64 기준 ms)과 예약 데이터. operator<가 반대라 priority_queue의 맨 위가 가장 이른 항목이다.
struct TimerItem
{
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

// DoTimer로 예약한 잡을 시각순으로 들고 있다가 때가 되면 해당 JobQueue에 넣는다.
// 전역 객체 GJobTimer 하나만 있고, 워커 스레드가 루프마다 Distribute를 부른다.
class JobTimer
{
public:
	// 지금부터 tickAfter(ms) 뒤에 owner에 넣을 잡을 예약한다. 여러 스레드에서 불러도 된다.
	void			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
	// now까지 만기된 잡을 owner의 Push로 넣는다. 한 번에 한 스레드만 실행하고, 나머지는 바로 리턴한다.
	// owner가 이미 소멸했으면 그 잡은 버린다. Push가 그 자리에서 잡을 실행할 수 있다.
	void			Distribute(uint64 now);
	// 남은 예약을 모두 버린다.
	void			Clear();

private:
	MAKE_LOCK;
	priority_queue<TimerItem>	_items;
	atomic<bool>				_distributing = false;
};

