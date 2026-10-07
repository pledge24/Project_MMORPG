#pragma once
#include <functional>

/*----------------
		Job
-----------------*/

using CallbackType = std::function<void()>;

/** 
 * 작업의 단위를 정의한 클래스. 콜백 함수를 만들어 들고있는 래퍼 클래스 형태이다.
 * 직접 생성하는 경우보다 JobQueue의 멤버 함수로 생성되는 경우가 훨씬 많다.
 */
class Job
{
public:
	/** 람다 버전. 캡처한 것의 수명은 넣는 쪽이 책임진다. */
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}

	/** 멤버 함수 버전. owner의 shared_ptr를 캡처하므로 잡이 실행될 때까지 owner가 살아 있다. */
	template<typename T, typename Ret, typename... Params>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Params...), Params&&... params)
	{
		_callback = [owner, memFunc, params...]()
		{
			(owner.get()->*memFunc)(params...);
		};
	}

	void Execute()
	{
		_callback();
	}

private:
	CallbackType _callback;
};

