#pragma once

/*------------------
	LockQueue
-------------------*/

// 뮤텍스 하나로 감싼 큐. 여러 스레드가 동시에 넣고 꺼내도 된다.
// 비면 T()를 돌려주므로, T는 shared_ptr처럼 빈 값을 구별할 수 있는 타입이어야 한다.
template<typename T>
class LockQueue
{
public:
	void Push(T item)
	{
		USE_LOCK;
		_items.push(item);
	}

	// 비어 있으면 T()를 돌려준다.
	T Pop()
	{
		USE_LOCK;
		return PopNoLock();
	}

	// 락을 잡지 않는다. 이미 락을 잡은 멤버 함수 안에서만 부른다.
	T PopNoLock()
	{
		if (_items.empty())
			return T();

		T ret = _items.front(); _items.pop();
		
		return ret;
	}

	// 남은 항목을 모두 items로 옮긴다. 중간에 T()가 들어 있으면 거기서 멈춘다.
	void PopAll(OUT vector<T>& items)
	{
		USE_LOCK;
		while (T item = PopNoLock())
			items.push_back(item);
	}

	void Clear()
	{
		USE_LOCK;
		_items = queue<T>();
	}

private:
	MAKE_LOCK;
	queue<T> _items;
};