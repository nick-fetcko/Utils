#pragma once

#include <algorithm>
#include <array>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <typeindex>
#include <vector>

namespace Fetcko {
class Utils {
private:
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> Utf8ToUtf16Wide;
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Utf8ToUtf16;

	static std::filesystem::path ResourceFolder;

public:
	static std::string GetStringFromFile(const std::filesystem::path &path);
	static void SetResourceFolder(const std::filesystem::path &path);
	static std::filesystem::path GetResourceFolder();
	static std::filesystem::path GetResource(const std::filesystem::path &path);
	static std::vector<std::filesystem::path> GetFiles(const std::filesystem::path &path);

	enum class BOM {UTF_8, UTF_16_BE, UTF_16_LE};

	template<typename C>
	static std::optional<BOM> GetBom(std::basic_istream<C> &in) {
		C chars[3];
		in.read(chars, 3);

		// If the input stream is less than 3
		// characters long, we have to clear
		// state.
		in.clear();

		const auto bytes = reinterpret_cast<const uint8_t *>(chars);

		if (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
			// FIXME: if C is wchar_t, we need to seek to character... 1.5
			return BOM::UTF_8;
		} else if (bytes[0] == 0xFE && bytes[1] == 0xFF) {
			if constexpr (std::is_same<C, wchar_t>::value)
				in.seekg(1);
			else if constexpr (std::is_same<C, char16_t>::value)
				in.seekg(1);
			else
				in.seekg(-1, std::ios::cur);

			return BOM::UTF_16_BE;
		} else if (bytes[0] == 0xFF && bytes[1] == 0xFE) {
			if constexpr (std::is_same<C, wchar_t>::value)
				in.seekg(1);
			else if constexpr (std::is_same<C, char16_t>::value)
				in.seekg(1);
			else
				in.seekg(-1, std::ios::cur);

			return BOM::UTF_16_LE;
		}

		in.seekg(0, std::ios::beg);

		return std::nullopt;
	}

	template<typename T>
	static std::vector<std::basic_string<T>> Split(const std::basic_string<T> &s, T delimiter) {
		std::vector<std::basic_string<T>> tokens;
		std::basic_stringstream<T> stream(s);
		std::basic_string<T> token;

		while (std::getline(stream, token, delimiter)) {
			tokens.emplace_back(std::move(token));
		}

		return tokens;
	}

	template<typename T>
	static std::vector<std::basic_string<T>> SplitOnce(const std::basic_string<T> &s, T delimiter) {
		if (auto pos = s.find_first_of(delimiter); pos != std::basic_string<T>::npos) {
			return {
				s.substr(0, pos),
				s.substr(pos + 1)
			};
		} else return { s };
	}

	template<typename T>
	static std::vector<std::basic_string<T>> Split(const std::basic_string<T> &s, int(*f)(int)) {
		// The is...() functions (which are the expected 2nd argument)
		// only function on ASCII chars. In MSVC, at least, an assert
		// is triggered if we try to pass non-ASCII chars in debug mode.
		const auto wrapper = [f](int c) {
			if (c < -1 || c > 255)
				return false;

			return f(c) != 0;
		};

		std::vector<std::basic_string<T>> tokens;

		std::size_t index = 0;
		std::size_t lastIndex = 0;
		while (index < s.size()) {
			// Skip any preceding delimiter characters
			while (index < s.size() && wrapper(s.at(index)))
				lastIndex = ++index;

			while (!wrapper(s.at(index))) {
				if (++index >= s.size())
					break;
			}
			tokens.emplace_back(s.substr(lastIndex, index - lastIndex));

			// Skip any remaining delimiter characters
			while (index < s.size() && wrapper(s.at(index))) ++index;

			lastIndex = index;
		}

		return tokens;
	}

	// From https://stackoverflow.com/a/217605

	// trim from start (in place)
	template<typename C>
	static inline void ltrim(std::basic_string<C> &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](C ch) {
			if (ch < -1 || ch > 255)
				return true;

			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	template <typename C>
	static inline void rtrim(std::basic_string<C> &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](C ch) {
			if (ch < -1 || ch > 255)
				return true;

			return !std::isspace(ch);
		}).base(), s.end());
	}

	constexpr static std::size_t GetNumberOfDigits(std::size_t size) {
		std::size_t digits = 1;
		while (size /= 10) ++digits;

		return digits;
	}

	template<typename T>
	static T LittleEndian(T t) {
		T ret = 0;
		for (std::size_t i = 0; i < sizeof(T); ++i)
			ret |= ((t >> ((sizeof(T) - i - 1) * 8)) & (0xFF)) << (i * 8);
		return ret;
	}

	// https://gist.github.com/Jookia/7a9b513e6e63de9e4bc0f52da7c7ef7e
	enum class Encoding {
		Ascii,
		Windows1252,
		ShiftJis
	};

	// This DOES NOT detect UTF-8 / UTF-16 / UTF-32
	// It's meant to be used only when Unicode fails
	static Encoding GuessEncoding(const std::string &str, std::size_t matches = 5) {
		std::size_t extendedAsciiChars = 0;
		std::size_t shiftJisChars = 0;

		for (std::size_t i = 0; i < str.size(); ++i) {
			uint8_t c = str[i];

			// Might be a Shift-JIS codepoint
			if ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xEF)) {
				if (i + 1 < str.size() - 1) {
					uint8_t next = str[i + 1];

					if (next >= 0x40 && next <= 0x9E && next != 0x7F) {
						++i; // Skip next byte
						++shiftJisChars;
					} else if (next >= 0x9F && next <= 0xFC) {
						++i; // Skip next byte
						++shiftJisChars;
					} else ++extendedAsciiChars;
				} else ++extendedAsciiChars;
			} else if (c > 127) ++extendedAsciiChars;
		}

		if (shiftJisChars >= matches) return Encoding::ShiftJis;
		else if (extendedAsciiChars >= matches) return Encoding::Windows1252;
		else return Encoding::Ascii;
	}

	static std::pair<bool, int> ExtractDigitsFromString(const std::string &str, bool onlyAtStart = false) {
		const char *digits = "0123456789";
		if (auto start = str.find_first_of(digits); (onlyAtStart && start == 0) || (!onlyAtStart && start != std::string::npos))
			return std::make_pair(true, std::stoi(str.substr(start)));
		else
			return std::make_pair(false, std::numeric_limits<int>::max());
	}

	static std::wstring ToUTF16(const std::string &utf8) {
		return Utf8ToUtf16Wide.from_bytes(utf8);
	}

	static std::string ToUTF8(const std::wstring &utf16) {
		return Utf8ToUtf16Wide.to_bytes(utf16);
	}

	static std::string ToUTF8(const std::u16string &utf16) {
		return Utf8ToUtf16.to_bytes(utf16);
	}

	static std::string GetFriendlyBytes(const std::size_t bytes) {
		constexpr std::array<std::string_view, 6> Suffixes = {
			"B",
			"KB",
			"MB",
			"GB",
			"TB",
			"PB"
		};

		double div = bytes;
		std::size_t index = 0;
		while (div >= 1024.0 && index < Suffixes.size() - 1) {
			++index;
			div /= 1024.0;
		}

		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << div << " " << Suffixes[index];

		return stream.str();
	}

	// From http://reedbeta.com/blog/python-like-enumerate-in-cpp17/
	template<typename T,
		typename TIter = decltype(std::begin(std::declval<T>())),
		typename = decltype(std::end(std::declval<T>()))>
	constexpr static auto Enumerate(T &&iterable) {
		struct iterator {
			size_t i;
			TIter iter;
			bool operator!=(const iterator &rhs) const { return iter != rhs.iter; }
			void operator++() { ++i; ++iter; }
			auto operator*() const { return std::tie(i, *iter); }
		};
		struct iterable_wrapper {
			T iterable;
			auto begin() { return iterator{ 0, std::begin(iterable) }; }
			auto end() { return iterator{ 0, std::end(iterable) }; }
		};
		return iterable_wrapper{ std::forward<T>(iterable) };
	}
};
}
