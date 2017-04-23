#pragma once

#include "log.h"
#include "exception.h"
#include <string>
#include <list>
#include <memory>

void stringToWString(const std::string& in, const std::wstring& out);
std::wstring stringToWString(const std::string& in);

inline void ltrim(std::string& str);
inline void rtrim(std::string& str);
inline void trim(std::string& str);

std::list<std::string> splitString(const std::string& str,
		const std::string& seperator);
std::shared_ptr<std::list<std::string>> splitString(const std::string& str,
		const std::string& seperators, const std::string::size_type maxPartLen);

bool stringEndsWith(const std::string& str, const std::string& searchFor);
bool stringStartsWith(const std::string& str, const std::string& searchFor);
bool stringEqualsIgnoreCase(const std::string& str1, const std::string& str2);

template<typename T> std::shared_ptr<T> sharedArray(T* array) {
	return std::shared_ptr<T>(array, std::default_delete<T[]>());
}
std::shared_ptr<char> copyStringToArray(const std::string& str);

std::string convertToHex(const unsigned char* in, const unsigned int arrayLen);
