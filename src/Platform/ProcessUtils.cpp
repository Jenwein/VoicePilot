#include "ProcessUtils.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
namespace Razel {

	std::string ProcessUtils::Exec(const char* cmd) {
		std::string command = "cmd /c \"" + std::string(cmd) + "\"";
		auto pipe = _popen(command.c_str(), "r");
		if (!pipe) {
			throw std::runtime_error("Failed to open pipe for command execution!");
		}

		std::array<char, 128> buffer;
		std::string result;

		// 循环读取管道中的数据,直到读完为止
		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
			result += buffer.data();
		}

		// 根据不同平台关闭管道
		_pclose(pipe);
		
		return result;
	}

}