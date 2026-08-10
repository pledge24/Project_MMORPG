#include "pch.h"
#include "RecvBuffer.h"

/*-------------------
	  RecvBuffer
--------------------*/

RecvBuffer::RecvBuffer(int32 chunkSize, int32 multipleN) : 
	_chunkSize(chunkSize), _multipleN(max(multipleN, MIN_MULTIPLE_N))
{
	_bufferSize = _chunkSize * _multipleN;	
	_buffer.resize(_bufferSize);
}

RecvBuffer::~RecvBuffer()
{
}

// [--------XXXX__________] ('--': 읽은 메모리, 'XXX': 안읽은 메모리, '__': free 메모리 )
// [XXXX__________________]
void RecvBuffer::Clean()
{
	int32 dataSize = UnreadSize();
	if (dataSize == 0)
	{
		_readPos = _writePos = 0;
	}
	else
	{
		// 여유 공간이 청크 1개 크기 미만이면, 데이터를 앞으로 당긴다.
		if (FreeSize() < _chunkSize)
		{
			::memcpy(&_buffer[0], &_buffer[_readPos], dataSize);
			_readPos = 0;
			_writePos = dataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 readBytes)
{
	if (UnreadSize() < readBytes)
		return false;

	_readPos += readBytes;
	return true;
}

bool RecvBuffer::OnWrite(int32 writeBytes)
{
	if (FreeSize() < writeBytes)
		return false;

	_writePos += writeBytes;
	return true;
}