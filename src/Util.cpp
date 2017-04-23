#include "Util.h"
#include <cstdlib>
#include <functional>
#include <algorithm>

void stringToWString(const std::string& in, std::wstring& out) {
	out.reserve(in.size());
	std::mbstowcs(&out[0], in.c_str(), in.size());
	out.shrink_to_fit();
}

std::wstring stringToWString(const std::string& in) {
	std::wstring tmp;
	tmp.reserve(in.size());
	stringToWString(in, tmp);
	return tmp;
}

inline void ltrim(std::string& str) {
	str.erase(str.begin(),
			std::find_if(str.begin(), str.end(),
					std::not1(std::ptr_fun<int, int>(std::isspace))));
}
inline void rtrim(std::string& str) {
	str.erase(
			std::find_if(str.rbegin(), str.rend(),
					std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
			str.end());
}
inline void trim(std::string& str) {
	ltrim(str);
	rtrim(str);
}

std::list<std::string> splitString(const std::string& str,
		const std::string& seperator) {
	std::list<std::string> ret;
	size_t pos = -1;
	size_t lastPos = 0;
	while ((pos = str.find(seperator, pos + 1)) != std::string::npos) {
		std::string tmp = str.substr(lastPos, pos - lastPos);
		trim(tmp);
		if (!tmp.empty()) {
			ret.push_back(tmp);
		}
		lastPos = pos + 1;
	}
	std::string tmp = str.substr(lastPos);
	trim(tmp);
	if (!tmp.empty()) {
		ret.push_back(tmp);
	}
	return ret;
}

std::shared_ptr<std::list<std::string>> splitString(const std::string& str,
		const std::string& seperators,
		const std::string::size_type maxPartLen) {
	std::shared_ptr<std::list<std::string>> ret(new std::list<std::string>());
	std::string src(str);
	while (src.size() > maxPartLen) {
		std::string::size_type pos = src.find_last_of(seperators,
				maxPartLen - 1);
		if (pos > maxPartLen || pos == str.npos) {
			pos = maxPartLen; // Cant split before max length, just split by max length
		}
		ret->push_back(src.substr(0, pos));
		src.erase(0, pos);
	}
	ret->push_back(src);
	return ret;
}

bool stringEndsWith(const std::string& str, const std::string& searchFor) {
	if (str.empty() || searchFor.empty()) {
		return false;
	}
	return (str.find(searchFor, str.size() - searchFor.size()) != str.npos) ?
			true : false;
}

bool stringStartsWith(const std::string& str, const std::string& searchFor) {
	if (str.empty() || searchFor.empty()) {
		return false;
	}
	return str.find(searchFor, 0) == 0 ? true : false;
}

bool charEqualsIgnoreCase(char a, char b) {
	return std::tolower(a) == std::tolower(b);
}
bool stringEqualsIgnoreCase(const std::string& str1, const std::string& str2) {
	if (str1.length() == str2.length()
			&& std::equal(str1.begin(), str1.end(), str2.begin(),
					charEqualsIgnoreCase)) {
		return true;
	}
	return false;
}

std::shared_ptr<char> copyStringToArray(const std::string& str) {
	std::shared_ptr<char> ret = sharedArray(new char[str.size() + 1]);
	str.copy(ret.get(), str.size());
	(ret.get())[str.size()] = '\0';
	return ret;
}

std::string convertToHex(const unsigned char* array,
		const unsigned int arrayLen) {
	static char HEX_CHARS[] = "0123456789ABCDEF";
	std::string ret;
	ret.reserve(arrayLen * 2);
	for (unsigned int i = 0; i < arrayLen; ++i) {
		unsigned char value = array[i];
		ret += HEX_CHARS[value >> 4];
		ret += HEX_CHARS[value & 0x0F];
	}
	return ret;
}
