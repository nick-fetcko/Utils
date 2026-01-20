#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string_view>
#include <thread>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "Filesystem.hpp"

#ifdef WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>

	// These Windows macros conflict
	// with STL algorithms
	#undef min
	#undef max
#endif

namespace Fetcko {
enum class LogLevel {
	Info,
	Debug,
	Warning,
	Error
};
class Logger;
class LoggableClass {
public:
	LoggableClass();

	LoggableClass(std::string &&name) :
		LoggableClass() {
		this->name = std::move(name);
	}

	// Make sure destructor is virtual
	virtual ~LoggableClass();

	template<typename T, typename... Args>
	void Log(LogLevel level, T t, Args... args) const;

	template<typename T, typename... Args>
	void LogInfo(T t, Args... args) const;

	template<typename T, typename... Args>
	void LogDebug(T t, Args... args) const;

	template<typename T>
	void LogDebug(T t) const;

	template<typename T, typename... Args>
	void LogWarning(T t, Args... args) const;

	template<typename T, typename... Args>
	void LogError(T t, Args... args) const;

	virtual const std::string &GetName() const { return name; }

protected:
	Logger *logger = nullptr;

	std::string name;
};
class Logger {
public:
	using Command = std::function<void(const std::vector<std::string> &)>;

	static void AddCommands(std::map<std::string, Command> &&commands);
	static void ProcessCommands();

private:
	static std::thread StartReadThread();
	static std::thread readThread;
	static std::map<std::string, Command> commands;
	static std::queue<std::pair<Command, std::vector<std::string>>> commandQueue;

	static std::mutex mutex;
	static std::atomic<bool> processingCommands;

	static std::size_t maxClassNameWidth;

public:
	static LogLevel logLevel;

	void SetObject(LoggableClass *object);
	void SetLogLevel(LogLevel logLevel) { this->logLevel = logLevel; }

	template<typename T, typename... Args>
	void LogInfo(T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Info, t, args...);
		PrintPrompt(LogLevel::Info);
	}

	template<typename T, typename... Args>
	void LogDebug(T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Debug, t, args...);
		PrintPrompt(LogLevel::Debug);
	}

	template<typename T, typename... Args>
	void LogWarning(T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Warning, t, args...);
		PrintPrompt(LogLevel::Warning);
	}

	template<typename T, typename... Args>
	void LogError(T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Error, t, args...);
		PrintPrompt(LogLevel::Error);
	}

	static void SetOnClose(std::function<void()> &&f) { onClose = f; }
	static const std::function<void()> &GetOnClose() { return onClose; }

	static void OnDestroy() {
#ifdef WIN32
		FreeConsole();
#endif
	}

private:
	template <typename T>
	void Log(T t)
#ifndef __ANDROID__
	const
#endif
	{
#ifndef __ANDROID__
		std::cout << t;
#else
		stream << t;
#endif
	}

	template<typename T, typename... Args>
	void Log(LogLevel level, T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		if (level >= logLevel) {
			Log(level, t);
			Log(args...);
		}
	}

	template<typename T>
	void Log(LogLevel level, T t)
#ifndef __ANDROID__
	const
#endif
	{
		if (level >= logLevel) {
#ifdef WIN32
			SetConsoleTextAttribute(
				out,
				static_cast<WORD>(Colors.at(level))
			);
#endif

			const auto time = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::now()
			);

			const auto &name = object->GetName();

#ifndef __ANDROID__
			std::cout
				<< "\r"
#else
			stream
#endif
				<< "["
				<< Labels.at(level)
				<< "] ("
				<< std::put_time(std::localtime(&time), "%d%b%Y %H:%M:%S")
				<< ") "
				<< std::setw(maxClassNameWidth)
				<< std::left
				<< std::setfill(' ')
				<< typeid(*object).name();

			if (name.size()) {
#ifndef __ANDROID__
				std::cout
#else
				stream
#endif
					<< " ("
					<< name
					<< ")";
			}

			std::stringstream addressStream;
			addressStream << std::hex << object;
			auto address = addressStream.str();
			address.erase(0, address.find_first_not_of('0'));

#ifndef __ANDROID__
			std::cout
#else
			stream
#endif
				<< " [0x"
				<< std::hex
				<< address
				<< std::dec
				<< "]: "
				<< t;
		}
	}

	template<typename T, typename... Args>
	void Log(T t, Args... args)
#ifndef __ANDROID__
	const
#endif
	{
		if constexpr (std::is_same<T, std::filesystem::path>::value)
#ifndef __ANDROID__
			std::cout << t.u8string();
#else
			stream << t.u8string();
#endif
		else
#ifndef __ANDROID__
			std::cout << t;
#else
			stream << t;
#endif

		Log(args...);
	}

	void PrintPrompt(LogLevel level)
	// Can't be const on Android because
	// we need to reset the stream.
#ifndef __ANDROID__
	const
#endif
	{
		if (level < logLevel) {
#ifdef __ANDROID__
			stream.str("");
#endif
			return;
		}

#ifndef __ANDROID__
		std::cout << std::endl;
#else
		int priority = ANDROID_LOG_UNKNOWN;

		switch(level) {
			case LogLevel::Info:
				priority = ANDROID_LOG_INFO;
				break;
			case LogLevel::Debug:
				priority = ANDROID_LOG_DEBUG;
				break;
			case LogLevel::Warning:
				priority = ANDROID_LOG_WARN;
				break;
			case LogLevel::Error:
				priority = ANDROID_LOG_ERROR;
				break;
			default:
				priority = ANDROID_LOG_UNKNOWN;
				break;
		}

		__android_log_print(priority, Filesystem::GetAppName().c_str(), "%s\n", stream.str().c_str());
		stream.str("");
#endif

		if (commands.empty()) return;

#ifdef WIN32
		SetConsoleTextAttribute(
			out,
			static_cast<WORD>(WindowsConsoleColors::Default)
		);
#endif

		std::cout << " > ";
	}

#ifdef WIN32
	enum class WindowsConsoleColors {
		Black,
		DarkBlue,
		DarkGreen,
		DarkCyan,
		DarkRed,
		DarkMagenta,
		DarkYellow,
		Default,
		Grey,
		Blue,
		Green,
		Cyan,
		Red,
		Magenta,
		Yellow,
		White
	};

	static inline const std::map<LogLevel, WindowsConsoleColors> Colors = {
		{ LogLevel::Info, WindowsConsoleColors::DarkCyan },
		{ LogLevel::Debug, WindowsConsoleColors::DarkGreen },
		{ LogLevel::Warning, WindowsConsoleColors::DarkYellow },
		{ LogLevel::Error, WindowsConsoleColors::DarkRed }
	};

	static inline const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	static inline const std::map<LogLevel, std::string_view> Labels = {
		{ LogLevel::Info,		" Info  " },
		{ LogLevel::Debug,		" Debug " },
		{ LogLevel::Warning,	"Warning" },
		{ LogLevel::Error,		" Error " }
	};

	LoggableClass *object = nullptr;

#ifdef __ANDROID__
	std::stringstream stream;
#endif

	static std::function<void()> onClose;
};

inline LoggableClass::LoggableClass() {
	logger = new Logger();
	logger->SetObject(this);
}

inline LoggableClass::~LoggableClass() {
	delete logger;
}

template<typename T, typename... Args>
inline void LoggableClass::Log(LogLevel level, T t, Args... args) const {
	logger->Log(level, t, args...);
}

template<typename T, typename... Args>
inline void LoggableClass::LogInfo(T t, Args... args) const {
	logger->LogInfo(t, args...);
}

template<typename T, typename... Args>
inline void LoggableClass::LogDebug(T t, Args... args) const {
	logger->LogDebug(t, args...);
}

template<typename T>
inline void LoggableClass::LogDebug(T t) const {
	logger->LogDebug(t);
}

template<typename T, typename... Args>
inline void LoggableClass::LogWarning(T t, Args... args) const {
	logger->LogWarning(t, args...);
}

template<typename T, typename... Args>
inline void LoggableClass::LogError(T t, Args... args) const {
	logger->LogError(t, args...);
}

template<> 
inline void Logger::Log<std::filesystem::path>(std::filesystem::path t)
#ifndef __ANDROID__
const
#endif
{
#ifndef __ANDROID__
	std::cout << t.u8string();
#else
	stream << t.u8string();
#endif
}

class LoggableThread : public std::thread, public LoggableClass {
public:
	LoggableThread(std::string &&name, std::function<void()> &&f) : std::thread(std::move(f)) {
		this->name = std::move(name);
	}
	virtual ~LoggableThread() = default;
};
}