#include "ProcessUtils.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
namespace Razel {

	std::string ProcessUtils::Exec(const char* cmd) {
// 根据不同平台选择实现
#ifdef _WIN32
	// Windows specific implementation
		std::string command = "cmd /c \"" + std::string(cmd) + "\"";
		auto pipe = _popen(command.c_str(), "r");
		if (!pipe) {
			throw std::runtime_error("Failed to open pipe for command execution!");
		}
#else
	// POSIX specific implementation (Linux, macOS)
		auto pipe = popen(cmd, "r");
		if (!pipe) {
			throw std::runtime_error("Failed to open pipe for command execution!");
		}
#endif

		std::array<char, 128> buffer;
		std::string result;

		// 循环读取管道中的数据,直到读完为止
		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			result += buffer.data();
		}

		// 根据不同平台关闭管道
#ifdef _WIN32
		_pclose(pipe);
#else
		pclose(pipe);
#endif

		return result;
	}

}