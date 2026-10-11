#include "ServerCore/Core/pch.h"
#include "ServerCore/Job/JobTimer.h"
#include "ServerCore/Job/JobQueue.h"

/*--------------
	JobTimer
---------------*/

void JobTimer::Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job)
{
	const uint64 executeTick = ::GetTickCount64() + tickAfter;
	JobData* jobData = new JobData(owner, job);

    // pq 보호
	USE_LOCK

	_items.push(TimerItem{ executeTick, jobData });
}

void JobTimer::Distribute(uint64 now)
{
	// 한 번에 한 쓰레드만 통과
	if (_distributing.exchange(true) == true)
		return;

	vector<TimerItem> items;

	{
	    // pq 보호
		USE_LOCK

		while (_items.empty() == false)
		{
			const TimerItem& timerItem = _items.top();
			if (now < timerItem.executeTick)
				break;

			items.push_back(timerItem);
			_items.pop();
		}
	}

	// 넣기만 하고 실행은 글로벌 큐를 꺼내는 워커에 맡긴다. 여기서 실행하면 잡이 끝날 때까지 _distributing이 서 있어
	// 다른 워커가 분배를 건너뛰고 같은 묶음의 뒤쪽 잡도 늦는다.
	for (TimerItem& item : items)
	{
		if (JobQueueRef owner = item.jobData->owner.lock())
			owner->Push(item.jobData->job, true);

		delete item.jobData;		
	}

	// 끝났으면 풀어준다
	_distributing.store(false);
}

void JobTimer::Clear()
{
    // pq 보호
	USE_LOCK

	while (_items.empty() == false)
	{
		const TimerItem& timerItem = _items.top();
		delete timerItem.jobData;
		_items.pop();
	}
}
