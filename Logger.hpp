#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string_view>
#include <thread>

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
class LoggableClass;
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
	enum class Level {
		Info,
		Debug,
		Warning,
		Error
	};

	static Level logLevel;

	void SetObject(LoggableClass *object);
	void SetLogLevel(Level logLevel) { this->logLevel = logLevel; }

	template<typename T, typename... Args>
	void LogInfo(T t, Args... args) const {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands) lock.lock();

		Log(Level::Info, t, args...);
		PrintPrompt();
	}

	template<typename T, typename... Args>
	void LogDebug(T t, Args... args) const {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands) lock.lock();

		Log(Level::Debug, t, args...);
		PrintPrompt();
	}

	template<typename T, typename... Args>
	void LogWarning(T t, Args... args) const {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands) lock.lock();

		Log(Level::Warning, t, args...);
		PrintPrompt();
	}

	template<typename T, typename... Args>
	void LogError(T t, Args... args) const {
		std::unique_lock lock(mutex, std::defer_lock);

		if (!processingCommands) lock.lock();

		Log(Level::Error, t, args...);
		PrintPrompt();
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
	void Log(T t) const {
		std::cout << t;
	}

	template<typename T, typename... Args>
	void Log(Level level, T t, Args... args) const {
		if (level >= logLevel) {
			Log(level, t);
			Log(args...);
		}
	}

	template<typename T>
	void Log(Level level, T t) const {
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

			std::cout
				<< "\r["
				<< Labels.at(level)
				<< "] ("
				<< std::put_time(std::localtime(&time), "%d%b%Y %H:%M:%S")
				<< ") "
				<< std::setw(maxClassNameWidth)
				<< std::left
				<< std::setfill(' ')
				<< typeid(*object).name();

			if (name.size()) {
				std::cout
					<< " ("
					<< name
					<< ")";
			}

			std::stringstream addressStream;
			addressStream << std::hex << object;
			auto address = addressStream.str();
			address.erase(0, address.find_first_not_of('0'));

			std::cout
				<< " [0x"
				<< std::hex
				<< address
				<< std::dec
				<< "]: "
				<< t;
		}
	}

	template<typename T, typename... Args>
	void Log(T t, Args... args) const {
		std::cout << t;
		Log(args...);
	}

	void PrintPrompt() const {
		std::cout << std::endl;

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

	static const std::map<Level, WindowsConsoleColors> Colors;

	static const HANDLE out;
#endif

	static const std::map<Level, std::string_view> Labels;

	LoggableClass *object = nullptr;

	static std::function<void()> onClose;
};

class LoggableClass {
public:
	LoggableClass() {
		logger.SetObject(this);
	}

	LoggableClass(std::string &&name) :
		LoggableClass() {
		this->name = std::move(name);
	}

	// Make sure destructor is virtual
	virtual ~LoggableClass() = default;

	template<typename T, typename... Args>
	void Log(Logger::Level level, T t, Args... args) {
		logger.Log(level, t, args...);
	}

	template<typename T, typename... Args>
	void LogInfo(T t, Args... args) {
		logger.LogInfo(t, args...);
	}

	template<typename T, typename... Args>
	void LogDebug(T t, Args... args) {
		logger.LogDebug(t, args...);
	}

	template<typename T, typename... Args>
	void LogWarning(T t, Args... args) {
		logger.LogWarning(t, args...);
	}

	template<typename T, typename... Args>
	void LogError(T t, Args... args) {
		logger.LogError(t, args...);
	}

	virtual const std::string &GetName() const { return name; }

protected:
	Logger logger;

	std::string name;
};

class LoggableThread : public std::thread, public LoggableClass {
public:
	LoggableThread(std::string &&name, std::function<void()> &&f) : std::thread(std::move(f)) {
		this->name = std::move(name);
	}
	virtual ~LoggableThread() = default;
};
}