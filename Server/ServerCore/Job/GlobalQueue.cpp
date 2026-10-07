#include "ServerCore/Core/pch.h"
#include "ServerCore/Job/GlobalQueue.h"

/*----------------
	GlobalQueue
-----------------*/

GlobalQueue::GlobalQueue()
{

}

GlobalQueue::~GlobalQueue()
{

}

void GlobalQueue::Push(JobQueueRef jobQueue)
{
	_jobQueues.Push(jobQueue);
}

JobQueueRef GlobalQueue::Pop()
{
	return _jobQueues.Pop();
}
