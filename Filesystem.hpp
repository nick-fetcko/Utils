#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <Shlobj.h>
#endif

namespace Fetcko {
class Filesystem {
public:
	static void SetAppName(const std::string &appName);
	static const std::string &GetAppName();

	static void SetPath(const std::string &path);
	static std::filesystem::path GetPath(const std::string &fileName = "Settings.json");

private:
	static inline std::string path;
	static inline std::string appName;
};
}