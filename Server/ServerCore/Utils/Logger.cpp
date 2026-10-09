#include "ServerCore/Core/pch.h"
#include "ServerCore/Utils/Logger.h"

namespace
{
	/** 레벨 표시는 다섯 칸으로 맞춰 뒤따르는 스레드 표시의 열을 맞춘다. */
	string_view LevelLabel(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Debug:	return "DEBUG";
		case LogLevel::Info:	return "INFO ";
		case LogLevel::Warning:	return "WARN ";
		case LogLevel::Error:	return "ERROR";
		}

		return "?????";
	}
}

/*--------------
	 Logger
---------------*/

Logger::Logger(ostream& out) : _out(out)
{

}

void Logger::Write(LogLevel level, string_view message)
{
	// 줄은 락 밖에서 만들고, 락 안에서는 쓰기만 한다.
	string line = FormatLine(level, chrono::system_clock::now(), LThreadId, message);
	line.push_back('\n');

	USE_LOCK;
	_out.write(line.data(), static_cast<streamsize>(line.size()));
	_out.flush();
}

string Logger::FormatLine(LogLevel level, chrono::system_clock::time_point time, uint32 threadId, string_view message)
{
	const time_t seconds = chrono::system_clock::to_time_t(time);
	tm local = {};
	::localtime_s(&local, &seconds);

	const int64 milliseconds = chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch()).count() % 1000;

	return format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03} [{}] [T{}] {}",
		local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		local.tm_hour, local.tm_min, local.tm_sec, milliseconds,
		LevelLabel(level), threadId, message);
}
