#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

namespace Fetcko {
class ShiftJIS {
private:
	// Defined in ShiftJIS.cpp
	const static std::array<uint8_t, 25088> LookupTable;

public:
	// Derived from https://stackoverflow.com/a/33170901
	static std::string ToUtf8(const std::string &input) {
		std::string output(3 * input.length(), ' '); // ShiftJis won't give 4byte UTF8, so max. 3 byte per input char are needed
		size_t indexInput = 0, indexOutput = 0;

		while (indexInput < input.length()) {
			char arraySection = static_cast<uint8_t>(input[indexInput]) >> 4;

			size_t arrayOffset;
			if (arraySection == 0x8) arrayOffset = 0x100; // these are two-byte shiftjis
			else if (arraySection == 0x9) arrayOffset = 0x1100;
			else if (arraySection == 0xE) arrayOffset = 0x2100;
			else arrayOffset = 0; //this is one byte shiftjis

			// determining real array offset
			if (arrayOffset) {
				arrayOffset += ((static_cast<std::size_t>(input[indexInput]) & 0xf)) << 8;
				indexInput++;
				if (indexInput >= input.length()) break;
			}
			arrayOffset += static_cast<uint8_t>(input[indexInput++]);
			arrayOffset <<= 1;

			// unicode number is...
			uint16_t unicodeValue = (LookupTable[arrayOffset] << 8) | LookupTable[arrayOffset + 1];

			// converting to UTF8
			if (unicodeValue < 0x80) {
				output[indexOutput++] = unicodeValue;
			} else if (unicodeValue < 0x800) {
				output[indexOutput++] = 0xC0 | (unicodeValue >> 6);
				output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
			} else {
				output[indexOutput++] = 0xE0 | (unicodeValue >> 12);
				output[indexOutput++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
				output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
			}
		}

		output.resize(indexOutput); // remove the unnecessary bytes
		return output;
	}
};
}