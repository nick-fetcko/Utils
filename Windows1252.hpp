#pragma once

#include <array>
#include <string>

namespace Fetcko {
class Windows1252 {
private:
	constexpr static std::array<wchar_t, 128> LookupTable = {
		u'\u20AC', u'\uFFFD', u'\u201A', u'\u0192', u'\u201E', u'\u2026', u'\u2020', u'\u2021',
		u'\u02C6', u'\u2030', u'\u0160', u'\u2039', u'\u0152', u'\uFFFD', u'\u017D', u'\uFFFD',
		u'\uFFFD', u'\u2018', u'\u2019', u'\u201C', u'\u201D', u'\u2022', u'\u2013', u'\u2014',
		u'\u02DC', u'\u2122', u'\u0161', u'\u203A', u'\u0153', u'\uFFFD', u'\u017E', u'\u0178',
		u'\u00A0', u'\u00A1', u'\u00A2', u'\u00A3', u'\u00A4', u'\u00A5', u'\u00A6', u'\u00A7',
		u'\u00A8', u'\u00A9', u'\u00AA', u'\u00AB', u'\u00AC', u'\u00AD', u'\u00AE', u'\u00AF',
		u'\u00B0', u'\u00B1', u'\u00B2', u'\u00B3', u'\u00B4', u'\u00B5', u'\u00B6', u'\u00B7',
		u'\u00B8', u'\u00B9', u'\u00BA', u'\u00BB', u'\u00BC', u'\u00BD', u'\u00BE', u'\u00BF',
		u'\u00C0', u'\u00C1', u'\u00C2', u'\u00C3', u'\u00C4', u'\u00C5', u'\u00C6', u'\u00C7',
		u'\u00C8', u'\u00C9', u'\u00CA', u'\u00CB', u'\u00CC', u'\u00CD', u'\u00CE', u'\u00CF',
		u'\u00D0', u'\u00D1', u'\u00D2', u'\u00D3', u'\u00D4', u'\u00D5', u'\u00D6', u'\u00D7',
		u'\u00D8', u'\u00D9', u'\u00DA', u'\u00DB', u'\u00DC', u'\u00DD', u'\u00DE', u'\u00DF',
		u'\u00E0', u'\u00E1', u'\u00E2', u'\u00E3', u'\u00E4', u'\u00E5', u'\u00E6', u'\u00E7',
		u'\u00E8', u'\u00E9', u'\u00EA', u'\u00EB', u'\u00EC', u'\u00ED', u'\u00EE', u'\u00EF',
		u'\u00F0', u'\u00F1', u'\u00F2', u'\u00F3', u'\u00F4', u'\u00F5', u'\u00F6', u'\u00F7',
		u'\u00F8', u'\u00F9', u'\u00FA', u'\u00FB', u'\u00FC', u'\u00FD', u'\u00FE', u'\u00FF'
	};

public:
	static std::wstring ToUtf16(const std::string &win1252) {
		std::wstring ret;

		for (uint8_t c : win1252) {
			if (c >= 128) 
				ret += LookupTable[c - 128];
			else ret += static_cast<wchar_t>(c);
		}

		return ret;
	}
};
}