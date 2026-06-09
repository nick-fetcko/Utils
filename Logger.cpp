#include "Logger.hpp"

#include <cstring>
#include <string>
#include <thread>

#include "Utils.hpp"

using namespace std::chrono_literals;

#ifdef WIN32
// https://discourse.libsdl.org/t/detect-console-window-close-windows/20557/4
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_CLOSE_EVENT) {
		if (auto onClose = Fetcko::Logger::GetOnClose(); onClose) {
			onClose();

			if (auto isClosed = Fetcko::Logger::GetIsClosed(); isClosed) {
				while (!isClosed())
					std::this_thread::sleep_for(1ms);
			}
		}

		return true;
	}
	return false;
}
#endif

namespace Fetcko {
// ===============================================
// =========== Initializing Statics ==============
// ===============================================
std::mutex Logger::mutex;
std::map<std::string, Logger::Command> Logger::commands;
std::queue<std::pair<Logger::Command, std::vector<std::string>>> Logger::commandQueue;
uint8_t Logger::fileIndex = 1;
#ifndef __ANDROID__
std::ofstream Logger::file = Logger::OpenFile();
#else
std::ofstream Logger::file;
#endif

std::size_t Logger::maxClassNameWidth = 0;

#if MULTITHREADED_LOGGING
std::thread Logger::StartWriteThread() {
	return std::thread([] {
		std::pair<LogLevel, std::string> message;
		writing = true;
		while (writing) {
			if (messageQueue.size()) {
				{
					std::unique_lock lock(queueMutex);
					message = std::move(messageQueue.front());
					messageQueue.pop();
				}

#ifdef WIN32
				SetConsoleTextAttribute(
					out,
					static_cast<WORD>(Colors.at(message.first))
				);
#endif

				std::cout << message.second << std::endl;

#ifdef WIN32
				SetConsoleTextAttribute(
					out,
					static_cast<WORD>(WindowsConsoleColors::Default)
				);
#endif

				// If we've exceeded the max file size,
				// open the next log file
				if (file.is_open()) {
					if (file.tellp() > MaxFileSize)
						file = OpenNextFile();

					file <<
#ifndef __ANDROID__
						// Remove leading carriage return
						message.second.substr(1)
#else
						message
#endif
						<< std::endl;

				}
			}

			std::this_thread::sleep_for(5ms);
		}
	});
}
std::queue<std::pair<LogLevel, std::string>> Logger::messageQueue;
std::mutex Logger::queueMutex;
bool Logger::writing = false;
std::thread Logger::writeThread = Logger::StartWriteThread();
Logger Logger::globalLogger;
#endif

#if defined(_DEBUG) && !MULTITHREADED_LOGGING
std::thread Logger::StartReadThread() {
	std::thread ret { [] {
		std::string line;
		while (true) {
			std::getline(std::cin, line);

			if (auto split = Fetcko::Utils::Split(line, ' '); !split.empty()) {
				std::unique_lock lock(mutex);
				if (auto iter = commands.find(split[0]); iter != commands.end())
					commandQueue.emplace(std::make_pair(iter->second, split));// iter->second(split);
			}
			std::cout << " > ";
		}
	} };

	// There's no way to interrupt std::getline,
	// so we can never reliably join this thread.
	// Instead, we'll let it die when the process
	// ends.
	ret.detach();

	return ret;
}

std::thread Logger::readThread = Logger::StartReadThread();
#endif

std::atomic<bool> Logger::processingCommands = false;

void Logger::ProcessCommands() {
	{
		std::unique_lock lock(mutex);
		processingCommands = true;
		while (!commandQueue.empty()) {
			auto &[f, split] = commandQueue.front();
			f(split);
			commandQueue.pop();
		}
	}
	processingCommands = false;
}

LogLevel Logger::logLevel = LogLevel::Debug;

std::function<void()> Logger::onClose;
std::function<bool()> Logger::isClosed;

// ===============================================
// ============= Member Functions ================
// ===============================================
#if MULTITHREADED_LOGGING
Logger::~Logger() {
	// Use our static logger
	// to determine when the
	// last logger is destructed
	if (this == &globalLogger) {
		writing = false;
		if (writeThread.joinable())
			writeThread.join();
	}
}
#endif

void Logger::AddCommands(std::map<std::string, Command> &&commands) {
	std::unique_lock lock(mutex);

	// When we first add commands, initialize a console window
	if (Logger::commands.empty() && !commands.empty()) {
#if defined(WIN32) && defined(_DEBUG)
		AllocConsole();
		AttachConsole(ATTACH_PARENT_PROCESS);

		SetConsoleTitleA("Debug Console");

		SetConsoleCtrlHandler(ConsoleHandlerRoutine, true);
#endif
	}
	Logger::commands.merge(std::move(commands));
}

void Logger::SetObject(LoggableClass *object) {
	this->object = object;

	std::unique_lock lock(mutex, std::defer_lock);

	if (!processingCommands) lock.lock();

	if (auto width = std::strlen(typeid(*object).name()); width > maxClassNameWidth)
		maxClassNameWidth = width;
}

std::ofstream Logger::OpenFile() {
	const auto path = Filesystem::GetPath("log");

	if (!std::filesystem::exists(path))
		std::filesystem::create_directory(path);

	uint8_t index = 1;
	for (; index <= MaxFiles; ++index) {
		if (!std::filesystem::exists(path / (std::to_string(index) + ".log"))) {
			// If we have no files yet, create our first
			if (index != 1)
				--index;

			// If we've not hit the max number of files,
			// use the last existing one
			break;
		}
	}

	// If we've reached the maximum number of log files,
	// find the one that was written to _last_
	auto lastWriteTime = std::filesystem::file_time_type();
	uint8_t lastWriteIndex = index;
	if (index > MaxFiles) {
		for (index = 1; index <= MaxFiles; ++index) {
			if (auto writeTime = std::filesystem::last_write_time(path / (std::to_string(index) + ".log")); writeTime > lastWriteTime) {
				lastWriteIndex = index;
				lastWriteTime = writeTime;
			}
		}
	}

	fileIndex = lastWriteIndex;

	return std::ofstream(path / (std::to_string(fileIndex) + ".log"), std::ios::out | std::ios::app);
}

std::ofstream Logger::OpenNextFile() {
	const auto path = Filesystem::GetPath("log");

	// Roll around to 1 once we've hit the max
	if ((++fileIndex) > MaxFiles)
		fileIndex = 1;

	return std::ofstream(path / (std::to_string(fileIndex) + ".log"), std::ios::out); // No append; new file
}
}