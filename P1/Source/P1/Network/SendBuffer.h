#pragma once

#include "CoreMinimal.h"

/** 생성된 패킷 핸들러가 읽고 쓰는 패킷 헤더. 서버의 헤더와 배치가 같아야 한다. */
struct PacketHeader
{
    /** 헤더를 포함한 패킷 전체의 바이트 수. */
    uint16 size;
    uint16 id;
};

/**
 * 보낼 패킷 하나를 담는 버퍼.
 *
 * 직렬화한 패킷을 담아 송신 스레드로 넘기는 단위가 된다.
 */
class SendBuffer
{
public:
    SendBuffer(int32 bufferSize);
    ~SendBuffer();

    //~ Write
public:
    /** Buffer()에 직접 직렬화한 뒤 불러 쓴 바이트 수를 확정한다. */
    void Close(uint32 writeSize);

    /** 버퍼 처음부터 덮어쓴다. 버퍼보다 길면 아무것도 쓰지 않고 false. */
    bool Copy(void* data, int32 len);
    /** 쓴 자리 뒤에 덧붙인다. 남은 공간이 모자라면 아무것도 쓰지 않고 false. */
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

    /** 지금까지 쓴 바이트 수. 다음에 쓸 위치이기도 하다. */
    int32 _writePos = 0;
};
