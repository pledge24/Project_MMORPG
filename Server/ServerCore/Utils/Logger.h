#pragma once
#include <chrono>
#include <format>
#include <string_view>

/** 로그 한 줄의 심각도. */
enum class LogLevel : uint8
{
	Debug,
	Info,
	Warning,
	Error,
};

/*--------------
	 Logger
---------------*/

/**
 * 레벨, 시각, 스레드 id를 붙여 한 줄씩 출력 스트림에 쓰는 로거.
 * 여러 스레드가 동시에 써도 한 줄을 통째로 쓰므로 줄이 섞이지 않는다.
 * 현재 전역 객체인 GLogger 하나가 표준 출력에 쓴다.
 */
class Logger
{
public:
	explicit Logger(ostream& out);

	/** [LOCK] 지금 시각과 이 스레드의 LThreadId를 붙여 한 줄을 쓴다. */
	void			Write(LogLevel level, string_view message);

	/** [LOCK] Write(LogLevel::Debug, ...)에 std::format 인자를 받는다. */
	template<typename... Args>
	void			Debug(format_string<Args...> fmt, Args&&... args) { Write(LogLevel::Debug, format(fmt, forward<Args>(args)...)); }
	/** [LOCK] Write(LogLevel::Info, ...)에 std::format 인자를 받는다. */
	template<typename... Args>
	void			Info(format_string<Args...> fmt, Args&&... args) { Write(LogLevel::Info, format(fmt, forward<Args>(args)...)); }
	/** [LOCK] Write(LogLevel::Warning, ...)에 std::format 인자를 받는다. */
	template<typename... Args>
	void			Warning(format_string<Args...> fmt, Args&&... args) { Write(LogLevel::Warning, format(fmt, forward<Args>(args)...)); }
	/** [LOCK] Write(LogLevel::Error, ...)에 std::format 인자를 받는다. */
	template<typename... Args>
	void			Error(format_string<Args...> fmt, Args&&... args) { Write(LogLevel::Error, format(fmt, forward<Args>(args)...)); }

	/**
	 * 줄바꿈을 뺀 한 줄을 만든다. 시각은 로컬 시간으로 적는다.
	 * 형식: "2026-10-09 14:03:22.123 [INFO ] [T3] 메시지"
	 */
	static string	FormatLine(LogLevel level, chrono::system_clock::time_point time, uint32 threadId, string_view message);

private:
	MAKE_LOCK;
	ostream&		_out;
};
