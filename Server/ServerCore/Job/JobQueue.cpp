#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

/*----------------
      JobQueue
-----------------*/

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // USE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행까지 담당
	if (prevCount == 0)
	{
		// 이미 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// 들어있는 일감을 다른 쓰레드가 실행할 수 있도록 GlobalQueue에 떠넘긴다
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

void JobQueue::Execute()
{
    // LCurrentJobQueue 포인터는 소유권도 없고 역참조되지 않는 표시값으로 사용되기 때문에 
    // this를 사용해도 괜찮다. 역으로 shared_ptr로 변경 시 Queue 수명을 불필요하게 늘리고, 
    // weak_ptr로 변경 시 확인 비용이 추가되므로 오히려 손해다.
    LCurrentJobQueue = this;

	while (true)
	{
		vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs); // USE_LOCK

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		// 자리에 돌아왔을때 일감이 또 있는지 체크(없으면 나감)
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			return;
		}

		// 워라벨 체크
		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			// 남은 Job들을 다른 쓰레드가 처리할 수 있도록 GlobalQueue에 이 JobQueue의 참조를 넘긴다.
			GGlobalQueue->Push(shared_from_this());
			break;
		}			
	}
}
