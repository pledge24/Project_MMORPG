#pragma once

/*-------------------
	  RecvBuffer
--------------------*/

/**
 * 세션 하나의 TCP 수신 버퍼. 읽기 커서와 쓰기 커서 두 개로 쓰는 선형 버퍼다.
 * 크기는 chunkSize × multipleN이고 기본값은 64KB × 10 = 640KB다.
 * 쓰기 커서가 끝에 가까워지면 Clean이 안 읽은 데이터를 맨 앞으로 당긴다.
 */
class RecvBuffer
{
	enum { DEFAULT_MULTIPLE_N = 10, MIN_MULTIPLE_N = 2};
	/** 청크 하나의 기본 크기(64KB). */
	enum { DEFAULT_CHUNKSIZE = 0x10000};

public:
	RecvBuffer(int32 chunkSize = DEFAULT_CHUNKSIZE, int32 multipleN = DEFAULT_MULTIPLE_N);
	~RecvBuffer();

	//~ RecvBuffer 커서 관련
	/** 두 커서를 읽은 메모리 만큼 앞으로 당긴다. 메모리도 이에 맞게 재조정한다*/
	void			Clean();
	/** 처리한 만큼 읽기 커서를 옮긴다. 안 읽은 양보다 크면 false를 돌려준다. */
	bool			OnRead(int32 numOfBytes);
	/** 수신한 만큼 쓰기 커서를 옮긴다. 남은 공간보다 크면 false를 돌려준다. */
	bool			OnWrite(int32 numOfBytes);

	//~ RecvBuffer 정보 관련
	BYTE*			Buffer()			{ return _buffer.data(); }
	BYTE*			ReadPos()			{ return &_buffer[_readPos]; }
	BYTE*			WritePos()			{ return &_buffer[_writePos]; }
	int32			Len()				{ return (int32)_buffer.size(); }
	int32			UnreadSize()		{ return _writePos - _readPos; }
	int32			FreeSize()			{ return _bufferSize - _writePos; }
	int32			GetChunkSize()		{ return _chunkSize; }
	int32			GetMultipleN()		{ return _multipleN; }
	
private:
	vector<BYTE>	_buffer;
	int32			_chunkSize = 0;
	int32			_multipleN = 0;
	int32			_bufferSize = 0; 
	int32			_readPos = 0;
	int32			_writePos = 0;
};

