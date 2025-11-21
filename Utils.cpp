#include "Utils.hpp"

#include <iostream>
#include <string>
#include <sstream>

#include "Logger.hpp"

namespace Fetcko {
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Utils::Utf8ToUtf16Wide;
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Utils::Utf8ToUtf16;
std::filesystem::path Utils::ResourceFolder;

std::string Utils::GetStringFromFile(const std::filesystem::path & path) {
	// Derived from https://stackoverflow.com/a/525103
	std::ifstream inFile(path, std::ios::in | std::ios::binary);

	if (!inFile) {
		LoggableClass errorLog(typeid(Utils).name());
		errorLog.LogError("File ", path.u8string(), " not found");

		return "";
	}

	auto fileSize = std::filesystem::file_size(path);

	if (!fileSize) return "";

	std::vector<char> bytes(fileSize);
	inFile.read(bytes.data(), fileSize);

	return std::string(bytes.data(), fileSize);
}

void Utils::SetResourceFolder(const std::filesystem::path &path) {
	ResourceFolder = path;
}

std::filesystem::path Utils::GetResourceFolder() {
#ifdef _DEBUG
	return std::filesystem::path("..") / ".." / "Data";
#else
	return (ResourceFolder.empty() ? std::filesystem::path(".") : ResourceFolder) / "Data";
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