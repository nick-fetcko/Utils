#include "Logger.hpp"

#include <string>

#include "Utils.hpp"

#ifdef WIN32
// https://discourse.libsdl.org/t/detect-console-window-close-windows/20557/4
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_CLOSE_EVENT) {
		if (auto onClose = Fetcko::Logger::GetOnClose(); onClose)
			onClose();
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

std::size_t Logger::maxClassNameWidth = 0;

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

std::thread Logger::readThread = Logger::StartReadThread();

Logger::Level Logger::logLevel = Logger::Level::Debug;

#ifdef WIN32
const std::map<Logger::Level, Logger::WindowsConsoleColors> Logger::Colors = {
		{ Level::Info, WindowsConsoleColors::DarkCyan },
		{ Level::Debug, WindowsConsoleColors::DarkGreen },
		{ Level::Warning, WindowsConsoleColors::DarkYellow },
		{ Level::Error, WindowsConsoleColors::DarkRed }
};

const HANDLE Logger::out = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

const std::map<Logger::Level, std::string_view> Logger::Labels {
		{ Level::Info,		" Info  " },
		{ Level::Debug,		" Debug " },
		{ Level::Warning,	"Warning" },
		{ Level::Error,		" Error " }
};

std::function<void()> Logger::onClose;

// ===============================================
// ============= Member Functions ================
// ===============================================
void Logger::AddCommands(std::map<std::string, Command> &&commands) {
	std::unique_lock lock(mutex);

	// When we first add commands, initialize a console window
	if (Logger::commands.empty() && !commands.empty()) {
#ifdef WIN32
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
}