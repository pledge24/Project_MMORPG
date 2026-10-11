#include "Core/pch.h"
#include <gtest/gtest.h>
#include <regex>
#include <sstream>
#include <thread>
#include "ServerCore/Utils/Logger.h"

/*--------------------------------------------------------------
    로거 테스트

    로그는 여러 스레드(IOCP 워커, DB 스레드, 메인 스레드)가 한 콘솔에 함께 쓴다.
    어느 스레드가 언제 무엇을 남겼는지 줄만 보고 가려낼 수 있어야 하므로
    레벨, 시각, 스레드 표시의 형식을 고정하고, 동시에 써도 한 줄이 다른 줄과
    섞이지 않는지 본다.

    픽스처 결합도: 없음. 출력 스트림을 ostringstream으로 바꿔 끼운다.
---------------------------------------------------------------*/

namespace
{
    /** 로컬 시간 기준으로 시각을 만든다. FormatLine이 로컬 시간으로 적으므로 시간대와 무관하게 왕복한다. */
    chrono::system_clock::time_point MakeLocalTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 second, int32 millisecond)
    {
        tm local = {};
        local.tm_year = year - 1900;
        local.tm_mon = month - 1;
        local.tm_mday = day;
        local.tm_hour = hour;
        local.tm_min = minute;
        local.tm_sec = second;
        local.tm_isdst = -1;

        return chrono::system_clock::from_time_t(::mktime(&local)) + chrono::milliseconds(millisecond);
    }

    vector<string> SplitLines(const string& text)
    {
        vector<string> lines;
        istringstream stream(text);
        for (string line; getline(stream, line);)
            lines.push_back(line);

        return lines;
    }

    /** 테스트가 바꾼 LThreadId를 끝날 때 되돌린다. 다른 테스트가 메인 스레드의 id를 볼 수 있다. */
    class ThreadIdOverride
    {
    public:
        explicit ThreadIdOverride(uint32 threadId) : _saved(LThreadId) { LThreadId = threadId; }
        ~ThreadIdOverride() { LThreadId = _saved; }

    private:
        uint32 _saved;
    };
}

TEST(LoggerTest, LineHasTimeLevelThreadAndMessage)
{
    const auto time = MakeLocalTime(2026, 10, 9, 14, 3, 22, 123);

    EXPECT_EQ(Logger::FormatLine(LogLevel::Info, time, 3, "플레이어 입장"),
              "2026-10-09 14:03:22.123 [INFO ] [T3] 플레이어 입장");
}

TEST(LoggerTest, MillisecondsArePaddedToThreeDigits)
{
    const auto time = MakeLocalTime(2026, 1, 2, 3, 4, 5, 6);

    EXPECT_EQ(Logger::FormatLine(LogLevel::Info, time, 1, "a"), "2026-01-02 03:04:05.006 [INFO ] [T1] a");
}

TEST(LoggerTest, EachLevelHasItsOwnFiveCharacterLabel)
{
    const auto time = MakeLocalTime(2026, 10, 9, 0, 0, 0, 0);

    EXPECT_EQ(Logger::FormatLine(LogLevel::Debug, time, 1, "m"), "2026-10-09 00:00:00.000 [DEBUG] [T1] m");
    EXPECT_EQ(Logger::FormatLine(LogLevel::Info, time, 1, "m"), "2026-10-09 00:00:00.000 [INFO ] [T1] m");
    EXPECT_EQ(Logger::FormatLine(LogLevel::Warning, time, 1, "m"), "2026-10-09 00:00:00.000 [WARN ] [T1] m");
    EXPECT_EQ(Logger::FormatLine(LogLevel::Error, time, 1, "m"), "2026-10-09 00:00:00.000 [ERROR] [T1] m");
}

TEST(LoggerTest, WriteUsesCallingThreadIdAndEndsLine)
{
    ostringstream out;
    Logger logger(out);
    ThreadIdOverride threadId(7);

    logger.Warning("몬스터 {} 템플릿 없음", 42);

    const vector<string> lines = SplitLines(out.str());
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_TRUE(out.str().ends_with("\n"));
    EXPECT_TRUE(lines[0].ends_with(" [WARN ] [T7] 몬스터 42 템플릿 없음")) << lines[0];
}

TEST(LoggerTest, ConcurrentWritersDoNotInterleaveLines)
{
    constexpr int32 THREAD_COUNT = 8;
    constexpr int32 LINES_PER_THREAD = 500;

    ostringstream out;
    Logger logger(out);

    vector<thread> threads;
    for (int32 t = 0; t < THREAD_COUNT; t++)
    {
        threads.emplace_back([&logger, t]()
            {
                LThreadId = static_cast<uint32>(t + 1);
                for (int32 i = 0; i < LINES_PER_THREAD; i++)
                    logger.Info("writer={} line={}", t + 1, i);
            });
    }
    for (thread& th : threads)
        th.join();

    const regex linePattern(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[INFO \] \[T(\d+)\] writer=(\d+) line=(\d+)$)");

    const vector<string> lines = SplitLines(out.str());
    ASSERT_EQ(lines.size(), static_cast<size_t>(THREAD_COUNT * LINES_PER_THREAD));

    // 스레드마다 다음에 와야 할 줄 번호. 한 스레드 안의 순서도 지켜져야 한다.
    vector<int32> nextLine(THREAD_COUNT + 1, 0);
    for (const string& line : lines)
    {
        smatch match;
        ASSERT_TRUE(regex_match(line, match, linePattern)) << "섞인 줄: " << line;

        const int32 threadId = stoi(match[1].str());
        const int32 writer = stoi(match[2].str());
        ASSERT_EQ(threadId, writer) << "다른 스레드의 id가 붙었다: " << line;
        ASSERT_EQ(stoi(match[3].str()), nextLine[writer]) << line;
        nextLine[writer]++;
    }
}
