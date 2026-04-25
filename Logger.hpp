#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
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

	LoggableClass(LoggableClass &&other) noexcept {
		name = std::move(other.name);
		logger = other.logger;
		other.logger = nullptr;
	}

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

	LoggableClass &operator=(const LoggableClass &) = default;

protected:
	Logger *logger = nullptr;

	std::string name;
};
class Logger {
#ifdef __ANDROID__
friend class Filesystem;
#endif
public:
	using Command = std::function<void(const std::vector<std::string> &)>;

	static void AddCommands(std::map<std::string, Command> &&commands);
	static void ProcessCommands();

private:
	constexpr inline static uint8_t MaxFiles = 5;
	constexpr inline static std::size_t MaxFileSize = 1024 * 1024; // 1MB

#ifdef _DEBUG
	static std::thread StartReadThread();
	static std::thread readThread;
#endif
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
	void LogInfo(T t, Args... args) {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Info, t, args...);
		PrintPrompt(LogLevel::Info);
	}

	template<typename T, typename... Args>
	void LogDebug(T t, Args... args) {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Debug, t, args...);
		PrintPrompt(LogLevel::Debug);
	}

	template<typename T, typename... Args>
	void LogWarning(T t, Args... args) {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Warning, t, args...);
		PrintPrompt(LogLevel::Warning);
	}

	template<typename T, typename... Args>
	void LogError(T t, Args... args) {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands.load()) lock.lock();

		Log(LogLevel::Error, t, args...);
		PrintPrompt(LogLevel::Error);
	}

	static void SetOnClose(std::function<void()> &&f) { onClose = f; }
	static const std::function<void()> &GetOnClose() { return onClose; }

	static void SetIsClosed(std::function<bool()> &&f) { isClosed = f; }
	static const std::function<bool()> &GetIsClosed() { return isClosed; }

	static void OnDestroy() {
#ifdef WIN32
		FreeConsole();
#endif
	}

private:
	static std::ofstream OpenFile();
	static std::ofstream OpenNextFile();

	template <typename T>
	void Log(T t) {
		stream << t;
	}

	template<typename T, typename... Args>
	void Log(LogLevel level, T t, Args... args)
	{
		if (level >= logLevel) {
			Log(level, t);
			Log(args...);
		}
	}

	template<typename T>
	void Log(LogLevel level, T t)
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
	
			stream
#ifndef __ANDROID__
				<< "\r"
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
				stream
					<< " ("
					<< name
					<< ")";
			}

			std::stringstream addressStream;
			addressStream << std::hex << object;
			auto address = addressStream.str();
			address.erase(0, address.find_first_not_of('0'));

			stream
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
	{
		if constexpr (std::is_same<T, std::filesystem::path>::value)
			stream << t.u8string();
		else
			stream << t;

		Log(args...);
	}

	void PrintPrompt(LogLevel level)
	// Can't be const on Android because
	// we need to reset the stream.
	{
		if (level < logLevel) {
			stream.str("");
			return;
		}

#ifndef __ANDROID__
		std::cout << stream.str() << std::endl;
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
#endif

		// If we've exceeded the max file size,
		// open the next log file
		if (file.is_open()) {
			if (file.tellp() > MaxFileSize)
				file = OpenNextFile();

			file <<
#ifndef __ANDROID__
				// Remove leading carriage return
				stream.str().substr(1)
#else
				stream.str()
#endif
				<< std::endl;

		}
		stream.str("");

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

	std::stringstream stream;

	static std::ofstream file;
	static uint8_t fileIndex;

	static std::function<void()> onClose;
	static std::function<bool()> isClosed;
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
inline void Logger::Log<std::filesystem::path>(std::filesystem::path t) {
	stream << t.u8string();
}

class LoggableThread : public std::thread, public LoggableClass {
public:
	LoggableThread(std::string &&name, std::function<void()> &&f) : std::thread(std::move(f)) {
		this->name = std::move(name);
	}
	virtual ~LoggableThread() = default;
};
}