#pragma once

#include <thread>
#include <functional>

/*------------------
	ThreadManager
-------------------*/

// 스레드를 관리하는 매니저. 스레드를 생성하고, 파괴하며, 
// 각 스레드가 실행해야할 작업을 넣어 실행한다.
class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void	Launch(function<void(void)> callback);
	void	Join();

	static void InitTLS();
	static void DestroyTLS();

	static void DoGlobalQueueWork();
	static void DistributeReservedJobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};

