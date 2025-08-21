#pragma once
#include <string>
#include <iostream>

namespace Common {

	inline auto ASSERT(bool cond, const std::string& msg) noexcept {
		if (!cond) [[unlikely]] {
			std::cerr << msg << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	inline auto FATAL(const std::string& msg) noexcept {
		std::cerr << msg << std::endl;
		exit(EXIT_FAILURE);
	}
}