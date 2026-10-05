#pragma once

#include <thread>
#include <functional>

/*------------------
	ThreadManager
-------------------*/

// 스레드를 만들고 끝날 때까지 기다리는 매니저. 전역 객체 GThreadManager 하나만 있다.
// 만든 스레드마다 TLS(LThreadId)를 초기화한다. 메인 스레드는 생성자에서 초기화한다.
// 정적 함수 DistributeReservedJobs와 DoGlobalQueueWork는 워커 루프가 매 바퀴 부르는 단계다.
class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	// callback을 실행할 스레드를 만든다. 스레드는 callback 앞뒤로 TLS를 초기화하고 정리한다.
	void	Launch(function<void(void)> callback);
	// 만든 스레드가 모두 끝날 때까지 기다린다. 소멸자도 부른다.
	void	Join();

	// 이 스레드의 LThreadId를 1부터 차례로 매긴다.
	static void InitTLS();
	static void DestroyTLS();

	// LEndTickCount까지 GlobalQueue에서 JobQueue를 꺼내 실행한다. 꺼낼 큐가 없으면 바로 리턴한다.
	static void DoGlobalQueueWork();
	// 지금 시각까지 만기된 예약 잡을 JobTimer에서 꺼내 각 JobQueue에 넣는다.
	static void DistributeReservedJobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};

