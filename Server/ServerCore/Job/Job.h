#pragma once
#include <functional>

/*----------------
		Job
-----------------*/

using CallbackType = std::function<void()>;

/** 나중에 실행할 함수 하나. JobQueue와 DBQueue에 넣는다. */
class Job
{
public:
	/** 람다 버전. 캡처한 것의 수명은 넣는 쪽이 책임진다. */
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}

	/** 함수 포인터 버전 */
	template<typename T, typename Ret, typename... Params>
	/** 멤버 함수 버전. owner의 shared_ptr를 캡처하므로 잡이 실행될 때까지 owner가 살아 있다. */
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

