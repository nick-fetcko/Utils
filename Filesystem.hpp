#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <Shlobj.h>
#endif

namespace Fetcko {
class Filesystem {
public:
	static void SetAppName(const std::string &appName) { 
		Filesystem::appName = appName; 
	}

	static void SetPath(const std::string &path) {
		Filesystem::path = path;
	}

	static std::filesystem::path GetPath(const std::string &fileName = "Settings.json") {
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

	static const std::string &GetAppName() { return appName; }

private:
	static inline std::string path;
	static inline std::string appName;
};
}