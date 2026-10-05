#pragma once

/*-------------------
	  SendBuffer
--------------------*/

/**
 * 직렬화한 패킷 하나를 담는 고정 길이 송신 버퍼. 앞에서부터 빈틈없이 채운다.
 * SendBufferRef로 여러 세션의 송신 큐가 함께 들고 있을 수 있으므로, Send에 넘긴 뒤에는 내용을 바꾸지 않는다.
 */
class SendBuffer
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

public:
	/** SerializeToArray로 버퍼에 직접 쓴 뒤, 쓴 길이를 확정한다. WriteSize가 이 값이 된다. */
	void			Close(uint32 writeSize);  

	//~ SendBuffer 정보 관련
	BYTE*			Buffer()							{ return _buffer.data(); }
	int32			Len()								{ return (int32)_buffer.size(); }
	int32			WriteSize()							{ return _writePos; }
	int32			FreeSize()							{ return (int32)_buffer.size() - _writePos; }
	BYTE*			WritePos()							{ return &_buffer[_writePos]; }

	//~ SendBuffer 데이터 조작 관련
	/**
	 * Copy는 처음부터 덮어쓰고 Append는 쓴 위치 뒤에 붙인다. 공간이 모자라면 false를 돌려준다.
	 * 지금은 부르는 곳이 없다.
	 */
	bool			Copy(void* data, int32 len);
	bool			Append(void* data, int32 len);

private:
	vector<BYTE>	_buffer;
	/** 다음에 쓸 위치. 지금까지 쓴 길이와 같다. */
	int32			_writePos = 0;
};

