#pragma once

#include <thread>
#include <functional>

/*------------------
	ThreadManager
-------------------*/

/**
 * 스레드를 관리하는 매니저. 전역 객체 GThreadManager 하나만 있다.
 * 정적 함수 DistributeReservedJobs와 DoGlobalQueueWork는 워커 루프가 매 바퀴 부르는 단계다.
 */
class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	/** callback을 실행할 스레드를 만든다. 스레드는 callback 앞뒤로 TLS를 초기화하고 정리한다. */
	void	Launch(function<void(void)> callback);
	void	Join();

	static void InitTLS();
	static void DestroyTLS();

	/** LEndTickCount까지 GlobalQueue에서 JobQueue를 꺼내 실행한다. 꺼낼 큐가 없으면 바로 리턴한다. */
	static void DoGlobalQueueWork();
	/** 지금 시각까지 만기된 예약 잡을 JobTimer에서 꺼내 각 JobQueue에 넣는다. */
	static void DistributeReservedJobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};

