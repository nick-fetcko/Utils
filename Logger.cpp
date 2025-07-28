#include "Logger.hpp"

#include <string>

#include "Utils.hpp"

namespace Fetcko {
// ===============================================
// =========== Initializing Statics ==============
// ===============================================
std::size_t Logger::maxClassNameWidth = 0;

// ===============================================
// ============= Member Functions ================
// ===============================================
void Logger::SetObject(LoggableClass *object) {
	this->object = object;

	std::unique_lock lock(mutex);
	if (auto width = std::strlen(typeid(*object).name()); width > maxClassNameWidth)
		maxClassNameWidth = width;
}
}