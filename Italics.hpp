#pragma once

#include <codecvt>
#include <locale>
#include <string>

#include "Utils.hpp"

namespace Fetcko {
class Italics {
public:
	static std::string ToItalics(const std::string &string) {
		auto utf32 = Utils::ToUTF32(string);

		for (auto &c : utf32) {
			if (c >= 'A' && c <= 'Z')
				c = c - 'A' + 0x1D63C;
			else if (c >= 'a' && c <= 'z')
				c = c - 'a' + 0x1D656;
		}

		return Utils::ToUTF8(utf32);
	}
};
}
