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

std::filesystem::path Utils::GetResourceFolder() {
#ifdef _DEBUG
	return std::filesystem::path("..") / ".." / "Data";
#else
	return std::filesystem::path(".") / "Data";
#endif
}

std::filesystem::path Utils::GetResource(const std::filesystem::path &path) {
	return GetResourceFolder() / path;
}

std::vector<std::filesystem::path> Utils::GetFiles(const std::filesystem::path &path) {
	std::vector<std::filesystem::path> ret;

	for (const auto &iter : std::filesystem::directory_iterator(path))
		ret.emplace_back(iter.path());

	return ret;
}
}