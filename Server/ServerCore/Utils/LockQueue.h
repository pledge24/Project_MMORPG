#pragma once

template<typename T>
class LockQueue
{
public:
	void Push(T item)
	{
		USE_LOCK;
		_items.push(item);
	}

	T Pop()
	{
		USE_LOCK;
		return PopNoLock();
	}

	T PopNoLock()
	{
		if (_items.empty())
			return T();

		T ret = _items.front(); _items.pop();
		
		return ret;
	}

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