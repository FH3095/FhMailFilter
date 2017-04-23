#pragma once

#include <stdexcept>

class IllegalStateError: public std::logic_error {
public:
	explicit IllegalStateError(const char* const msg) :
			std::logic_error(msg) {
	}
	explicit IllegalStateError(const std::string& msg) :
			std::logic_error(msg) {
	}
};

class IllegalArgumentError: public std::logic_error {
public:
	explicit IllegalArgumentError(const char* const msg) :
			std::logic_error(msg) {
	}
	explicit IllegalArgumentError(const std::string& msg) :
			std::logic_error(msg) {
	}
};
