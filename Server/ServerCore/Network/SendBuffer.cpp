#include "pch.h"
#include "SendBuffer.h"

/*-------------------
	  SendBuffer
--------------------*/

SendBuffer::SendBuffer(int32 bufferSize)
{
	_buffer.resize(bufferSize);
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize)
{
	_writePos = writeSize;
}

bool SendBuffer::Copy(void* data, int32 len)
{
	if (Len() < len)
		return false;

	::memcpy(Buffer(), data, len);
	_writePos = len;

	return true;
}

bool SendBuffer::Append(void* data, int32 len)
{
	if (FreeSize() < len)
		return false;

	::memcpy(WritePos(), data, len);
	_writePos += len;

	return true;
}
