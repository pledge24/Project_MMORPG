#pragma once
#include <functional>

/*----------------
		Job
-----------------*/

using CallbackType = std::function<void()>;

class Job
{
public:
	// function 버전
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}

	// 함수 포인터 버전
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

