#pragma once

/*-------------------
	  SendBuffer
--------------------*/

// TCP 송신 버퍼(고정 길이)
// [-----------___________] (O)
// [---__---_-____________] (X)
class SendBuffer
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

public:
					/* SerializeToArray() 호출시에만 사용 */
	void			Close(uint32 writeSize);  

					/* SendBuffer 정보 관련 */
	BYTE*			Buffer()							{ return _buffer.data(); }
	int32			Len()								{ return (int32)_buffer.size(); }
	int32			WriteSize()							{ return _writePos; }
	int32			FreeSize()							{ return (int32)_buffer.size() - _writePos; }
	BYTE*			WritePos()							{ return &_buffer[_writePos]; }

					/* RecvBuffer 데이터 조작 관련 */
	bool			Copy(void* data, int32 len);
	bool			Append(void* data, int32 len);

private:
	vector<BYTE>	_buffer;
	int32			_writePos = 0; // data가 들어있는 다음 위치를 가리킴.
};

