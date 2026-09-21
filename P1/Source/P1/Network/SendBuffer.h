#pragma once

#include "CoreMinimal.h"

struct PacketHeader
{
    uint16 size;    // 2 BYTE
    uint16 id;      // 2 BYTE
};

class SendBuffer
{
public:
    SendBuffer(int32 bufferSize);
    ~SendBuffer();

    //~ Write
public:
    /** SerializeToArray()를 부른 뒤에만 쓴다. */
    void Close(uint32 writeSize);

    bool Copy(void* data, int32 len);
    bool Append(void* data, int32 len);

    //~ Buffer Info
public:
    BYTE* Buffer() { return _buffer.GetData(); }
    int32 Len() { return _buffer.Num(); }
    int32 WriteSize() { return _writePos; }
    int32 FreeSize() { return _buffer.Num() - _writePos; }
    BYTE* WritePos() { return &_buffer[_writePos]; }

private:
    TArray<BYTE> _buffer;

    /** 데이터가 들어 있는 다음 위치를 가리킨다. */
    int32 _writePos = 0;
};
