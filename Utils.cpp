#include "Utils.hpp"

#include <iostream>
#include <string>
#include <sstream>

#include "Logger.hpp"

namespace Fetcko {
std::string Utils::GetStringFromFile(const std::filesystem::path & path) {
	std::ifstream inFile(path, std::ios_base::in | std::ios_base::binary);

	if (!inFile) {
		LoggableClass errorLog(typeid(Utils).name());
		errorLog.LogError("File ", path.string(), " not found");
		
		return "";
	}

	std::stringstream buffer;
	buffer << inFile.rdbuf();

	return buffer.str();
}

std::filesystem::path Utils::GetResource(const std::filesystem::path &path) {
#ifdef _DEBUG
	return std::filesystem::path("..") / ".." / "Data" / path;
#else
	return std::filesystem::path(".") / "Data" / path;
#endif
}
}