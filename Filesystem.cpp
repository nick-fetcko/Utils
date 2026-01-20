#include "Filesystem.hpp"

#include "Logger.hpp"

namespace Fetcko {
void Filesystem::SetAppName(const std::string &appName) {
	Filesystem::appName = appName;
}

void Filesystem::SetPath(const std::string &path) {
	Filesystem::path = path;

#ifdef __ANDROID__
	Logger::file = Logger::OpenFile();
#endif
}

std::filesystem::path Filesystem::GetPath(const std::string &fileName) {
	std::filesystem::path ret;

#ifdef _WIN32
	PWSTR folder;
	if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &folder) == S_OK) {
		ret = std::filesystem::path(folder);

		CoTaskMemFree(folder);
	}
#elif defined(__ANDROID__)
	ret = Filesystem::path;
#elif defined(__linux__)
	ret = std::filesystem::path(getenv("HOME")) / ".config";
	if (!std::filesystem::exists(ret))
		std::filesystem::create_directory(ret);
#endif

	if (!ret.empty()) {
		ret /= "Fetcko";
		if (!std::filesystem::exists(ret))
			std::filesystem::create_directory(ret);

		ret /= appName;
		if (!std::filesystem::exists(ret))
			std::filesystem::create_directory(ret);

		ret /= fileName;
	}

	return ret;
}

const std::string &Filesystem::GetAppName() { return appName; }
}