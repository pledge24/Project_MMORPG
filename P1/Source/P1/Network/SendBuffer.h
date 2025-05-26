#pragma once

#include "CoreMinimal.h"

struct PacketHeader
{
	uint16 size;	// 2 BYTE
	uint16 id; 		// 2 BYTE
};

/*-------------------
	  SendBuffer
--------------------*/

class SendBuffer
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

public:
					/* SerializeToArray() 호출시에만 사용 */
	void			Close(uint32 writeSize);  

					/* SendBuffer 정보 관련 */
	BYTE*			Buffer()							{ return _buffer.GetData(); }
	int32			Len()								{ return _buffer.Num(); }
	int32			WriteSize()							{ return _writePos; }
	int32			FreeSize()							{ return _buffer.Num() - _writePos; }
	BYTE*			WritePos()							{ return &_buffer[_writePos]; }

					/* RecvBuffer 데이터 조작 관련 */
	bool			Copy(void* data, int32 len);
	bool			Append(void* data, int32 len);

private:
	TArray<BYTE>	_buffer;
	int32			_writePos = 0; // data가 들어있는 다음 위치를 가리킴.
};


