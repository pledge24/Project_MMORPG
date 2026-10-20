#include "Core/pch.h"
#include <gtest/gtest.h>
#include <thread>
#include <unordered_set>
#include "Utils/SnowflakeIdGenerator.h"

/*--------------------------------------------------------------
    Snowflake ID 발급기 테스트

    캐릭터 ID와 아이템 UID는 DB의 기본 키라서 한 번이라도 겹치면 저장이
    실패하거나 다른 캐릭터의 행을 덮는다. 여러 룸 스레드가 동시에 부르고,
    시계가 뒤로 가거나 한 밀리초에 순번이 넘쳐도 겹치지 않고 커지기만
    해야 한다. 부팅할 때 DB에서 읽은 최댓값보다 뒤에서 시작하는지도 본다.

    픽스처 결합도: 없음. 시계를 주입해 시각을 테스트가 정한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 NODE_ID = 3;
    constexpr int64 MAX_SEQUENCE = (int64{ 1 } << 12) - 1;

    // 비트 배치를 발급기와 따로 적는다. 발급기의 분해 함수로 검사하면 둘이 함께 틀려도 통과한다.
    int64 MakeId(int64 elapsedMs, int64 nodeId, int64 sequence)
    {
        return (elapsedMs << 22) | (nodeId << 12) | sequence;
    }

    // 기준 시각에서 elapsedMs만큼 지난 Unix 밀리초.
    int64 AtElapsed(int64 elapsedMs)
    {
        return SnowflakeIdGenerator::EPOCH_UNIX_MS + elapsedMs;
    }
}

TEST(SnowflakeIdGeneratorTest, EpochIsStartOf2026Utc)
{
    // 2026-01-01T00:00:00Z. 1970년부터 56년, 그중 윤년 14번.
    EXPECT_EQ(SnowflakeIdGenerator::EPOCH_UNIX_MS, int64{ 20454 } * 24 * 60 * 60 * 1000);
}

TEST(SnowflakeIdGeneratorTest, PacksElapsedTimeNodeAndSequence)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(5); });

    const int64 id = generator.Next();

    EXPECT_EQ(id, MakeId(5, NODE_ID, 0));

    const SnowflakeIdGenerator::Parts parts = SnowflakeIdGenerator::Decompose(id);
    EXPECT_EQ(parts.elapsedMs, 5);
    EXPECT_EQ(parts.nodeId, NODE_ID);
    EXPECT_EQ(parts.sequence, 0);
}

// 41비트 시각을 다 써도 부호 비트는 0이다. DB의 BIGINT에 음수로 들어가면 정렬이 뒤집힌다.
TEST(SnowflakeIdGeneratorTest, SignBitStaysZeroAtLargestTimeAndNode)
{
    const int64 maxElapsedMs = (int64{ 1 } << 41) - 1;
    SnowflakeIdGenerator generator(SnowflakeIdGenerator::MAX_NODE_ID, [=] { return AtElapsed(maxElapsedMs); });

    const int64 id = generator.Next();

    EXPECT_GT(id, 0);
    EXPECT_EQ(id, MakeId(maxElapsedMs, SnowflakeIdGenerator::MAX_NODE_ID, 0));
}

TEST(SnowflakeIdGeneratorTest, SameMillisecondIncrementsSequence)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(10); });

    EXPECT_EQ(generator.Next(), MakeId(10, NODE_ID, 0));
    EXPECT_EQ(generator.Next(), MakeId(10, NODE_ID, 1));
    EXPECT_EQ(generator.Next(), MakeId(10, NODE_ID, 2));
}

TEST(SnowflakeIdGeneratorTest, NewMillisecondResetsSequence)
{
    int64 now = AtElapsed(10);
    SnowflakeIdGenerator generator(NODE_ID, [&now] { return now; });

    generator.Next();
    generator.Next();
    now = AtElapsed(11);

    EXPECT_EQ(generator.Next(), MakeId(11, NODE_ID, 0));
}

// 한 밀리초에 4096개를 넘게 발급하면 시계를 기다리지 않고 다음 밀리초를 미리 쓴다.
TEST(SnowflakeIdGeneratorTest, SequenceOverflowAdvancesTimeWithoutWaiting)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(10); });

    int64 last = 0;
    for (int64 i = 0; i <= MAX_SEQUENCE; i++)
        last = generator.Next();
    ASSERT_EQ(last, MakeId(10, NODE_ID, MAX_SEQUENCE));

    EXPECT_EQ(generator.Next(), MakeId(11, NODE_ID, 0));
    EXPECT_EQ(generator.Next(), MakeId(11, NODE_ID, 1)) << "시계가 따라오기 전까지는 앞당긴 시각에서 순번을 올린다";
}

// 시계가 뒤로 가도 마지막으로 쓴 시각에서 순번을 올린다. 기다리거나 실패하지 않는다.
TEST(SnowflakeIdGeneratorTest, ClockGoingBackwardStaysMonotonic)
{
    int64 now = AtElapsed(100);
    SnowflakeIdGenerator generator(NODE_ID, [&now] { return now; });

    const int64 before = generator.Next();
    now = AtElapsed(40);
    const int64 after = generator.Next();

    EXPECT_GT(after, before);
    EXPECT_EQ(after, MakeId(100, NODE_ID, 1));

    now = AtElapsed(101);
    EXPECT_EQ(generator.Next(), MakeId(101, NODE_ID, 0)) << "시계가 마지막 시각을 넘어서면 다시 시계를 따른다";
}

// 부팅할 때 DB에서 읽은 최댓값이 지금 시계보다 앞서 있으면(앞당긴 시각, 늦은 시계) 그 뒤에서 이어 간다.
TEST(SnowflakeIdGeneratorTest, StartAfterContinuesBehindStoredMaximum)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(50); });

    generator.StartAfter(MakeId(80, NODE_ID, 7));

    EXPECT_EQ(generator.Next(), MakeId(80, NODE_ID, 8));
}

TEST(SnowflakeIdGeneratorTest, StartAfterOlderMaximumFollowsClock)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(50); });

    generator.StartAfter(MakeId(20, NODE_ID, 7));

    EXPECT_EQ(generator.Next(), MakeId(50, NODE_ID, 0));
}

// DB가 비어 있으면 최댓값이 0이다. 0에서 시작해도 시계를 따른다.
TEST(SnowflakeIdGeneratorTest, StartAfterZeroFollowsClock)
{
    SnowflakeIdGenerator generator(NODE_ID, [] { return AtElapsed(50); });

    generator.StartAfter(0);

    EXPECT_EQ(generator.Next(), MakeId(50, NODE_ID, 0));
}

// 여러 룸 스레드가 동시에 불러도 겹치지 않는다. 실제 시계를 써서 순번이 넘치는 경우도 함께 지난다.
TEST(SnowflakeIdGeneratorTest, ConcurrentIssueHasNoDuplicates)
{
    constexpr int32 THREAD_COUNT = 8;
    constexpr int32 IDS_PER_THREAD = 20000;
    SnowflakeIdGenerator generator(NODE_ID);

    vector<vector<int64>> issued(THREAD_COUNT);
    vector<std::thread> threads;
    for (int32 t = 0; t < THREAD_COUNT; t++)
    {
        threads.emplace_back([&generator, &ids = issued[t]]
            {
                ids.reserve(IDS_PER_THREAD);
                for (int32 i = 0; i < IDS_PER_THREAD; i++)
                    ids.push_back(generator.Next());
            });
    }
    for (std::thread& thread : threads)
        thread.join();

    std::unordered_set<int64> unique;
    for (const vector<int64>& ids : issued)
    {
        for (size_t i = 1; i < ids.size(); i++)
            ASSERT_GT(ids[i], ids[i - 1]) << "한 스레드 안에서는 커지기만 한다";

        for (int64 id : ids)
        {
            ASSERT_EQ(SnowflakeIdGenerator::Decompose(id).nodeId, NODE_ID);
            unique.insert(id);
        }
    }
    EXPECT_EQ(unique.size(), size_t{ THREAD_COUNT } * IDS_PER_THREAD);
}
