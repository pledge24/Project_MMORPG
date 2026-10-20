#pragma once
#include <functional>

/**
 * 서버 노드마다 겹치지 않는 64비트 ID를 발급하는 Snowflake 발급기.
 *
 * 캐릭터 ID와 아이템 UID처럼 DB의 기본 키가 되는 값을 DB에 묻지 않고 서버가 정한다.
 * ID는 기준 시각부터 지난 밀리초 41비트, 노드 번호 10비트, 순번 12비트로 이뤄지고 발급 순서대로 커진다.
 * 여러 스레드가 동시에 불러도 된다.
 */
class SnowflakeIdGenerator
{
public:
    /** Unix 시각(밀리초)을 돌려준다. 테스트는 실제 시계 대신 정한 시각을 넘긴다. */
    using Clock = std::function<int64()>;

    /** ID의 세 부분. elapsedMs는 기준 시각부터 지난 밀리초다. */
    struct Parts
    {
        int64 elapsedMs = 0;
        int32 nodeId = 0;
        int32 sequence = 0;
    };

    /** 2026-01-01T00:00:00Z의 Unix 밀리초. */
    static constexpr int64 EPOCH_UNIX_MS = 1767225600000;
    static constexpr int32 MAX_NODE_ID = 1023;

    /** nodeId는 0~MAX_NODE_ID여야 한다. 검증하지 않으므로 설정 검사를 통과한 값을 넘긴다. */
    explicit SnowflakeIdGenerator(int32 nodeId, Clock clock = &NowUnixMs);

    /**
     * 이 노드가 이미 발급한 ID의 최댓값을 알려 그 뒤에서 이어 가게 한다. 발급한 적이 없으면 0.
     * 첫 발급 전에 부른다.
     */
    void StartAfter(int64 lastIssuedId);

    /** 다음 ID. 기다리지 않는다. */
    int64 Next();

    static Parts Decompose(int64 id);
    static int64 NowUnixMs();

private:
    int64 Compose(int64 elapsedMs, int64 sequence) const;

    const int32 _nodeId;
    const Clock _clock;
    /** 마지막으로 발급한 ID. 시각과 순번을 함께 담아 한 번의 CAS로 바꾼다. */
    atomic<int64> _lastId;
};
