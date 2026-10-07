#pragma once

/**
 * 접속 종료 저장이 끝나기 전에는 같은 계정의 입장 불러오기를 하지 않게 막는다.
 * 계정마다 저장 대기를 표시하고, 대기 중에 온 불러오기를 하나만 맡아 둔다.
 * 세션과 DB를 모른다. 맡아 둔 일을 언제 어디서 실행할지는 호출자가 정한다.
 * 여러 스레드(IOCP, DB 큐, 타이머 큐)에서 부르므로 모든 함수가 락을 잡는다. 전역 객체 GSaveGate 하나만 있다.
 */
class SaveGate
{
public:
    /** 저장 대기 중에 맡아 둔 입장 불러오기. 실행은 맡긴 쪽이 아니라 대기를 푼 쪽이 한다. */
    struct ParkedLoad
    {
        function<void()> run;    // 저장이 끝나면 실행할 불러오기
        function<void()> reject; // 기다리다 만료되면 실행할 입장 거절
    };

    enum class ParkResult
    {
        NOT_HELD, // 대기 중이 아니다. 맡지 않았으므로 호출자가 바로 불러온다
        PARKED,   // 맡았다. 호출자는 token으로 만료 타이머를 건다
        BUSY,     // 이미 맡아 둔 불러오기가 있다. 맡지 않았으므로 호출자가 거절한다
    };

    struct ParkTicket
    {
        ParkResult result = ParkResult::NOT_HELD;
        uint64 token = 0; // PARKED일 때만 쓴다
    };

    /** 저장 대기를 건다. 이미 대기 중이면 아무것도 하지 않는다. */
    void Hold(int64 userId);

    /** 대기 중인 계정이고 맡아 둔 불러오기가 없을 때만 load를 맡는다. 결과별 처리는 ParkResult에 있다. */
    ParkTicket Park(int64 userId, ParkedLoad load);

    /** 저장이 끝났을 때 부른다. 대기를 풀고 맡아 둔 불러오기가 있으면 돌려준다. */
    optional<ParkedLoad> Release(int64 userId);

    /**
     * 만료 타이머에서 부른다. token이 지금 맡아 둔 불러오기의 것이면 대기를 풀고 그 불러오기를 돌려준다.
     * 그사이 풀렸거나 다른 불러오기로 바뀌었으면 빈 값을 돌려준다.
     */
    optional<ParkedLoad> Expire(int64 userId, uint64 token);

private:
    struct Entry
    {
        optional<ParkedLoad> parked;
        uint64 token = 0;
    };

    MAKE_LOCK;
    map<int64, Entry> _entries; // 대기 중인 계정만 들어 있다
    uint64 _nextToken = 1;
};

extern SaveGate GSaveGate;
