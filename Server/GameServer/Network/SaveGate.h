#pragma once

/**
 * 계정마다 저장이 끝나기를 기다리는 표시.
 *
 * 접속 종료 저장이 끝나기 전에는 같은 계정의 입장 불러오기를 돌리지 않도록 막는다.
 * 대기 중에 온 불러오기는 하나만 맡아 두고, 그 일을 언제 어디서 돌릴지는 호출자가 정한다.
 */
class SaveGate
{
public:
    /** 저장 대기 중에 맡아 둔 입장 불러오기. 실행은 대기를 푼 쪽이 한다. */
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

    /** [LOCK] 저장 대기를 건다. 이미 대기 중이면 아무것도 하지 않는다. */
    void Hold(int64 userId);

    /** [LOCK] 대기 중인 계정이고 맡아 둔 불러오기가 없을 때만 load를 맡는다. 결과별 처리는 ParkResult에 있다. */
    ParkTicket Park(int64 userId, ParkedLoad load);

    /** [LOCK] 저장이 끝났을 때 부른다. 대기를 풀고 맡아 둔 불러오기가 있으면 돌려준다. */
    optional<ParkedLoad> Release(int64 userId);

    /**
     * [LOCK] 만료 타이머에서 부른다. token의 불러오기가 아직 맡겨져 있으면 꺼내 돌려주고, 아니면 nullopt.
     * 대기는 Release까지 남으므로 다음 불러오기는 다시 맡는다.
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
