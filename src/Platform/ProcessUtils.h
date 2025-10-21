#pragma once

#include <string>

// 提供平台相关的工具函数,提供一个通用的、与平台无关的方法来执行任何外部命令行进程并捕获其输出。
// 底层系统调用,启动一个进程（.exe, .py等）并读取其stdout。

namespace Razel {
	class ProcessUtils
	{
	public:
		/**
		 * @brief 执行一个外部命令并捕获其标准输出。
		 * @param cmd 要执行的完整命令行指令。
		 * @return 返回命令的标准输出内容。如果命令执行失败或没有输出，则返回空字符串。
		 * @throws std::runtime_error 如果无法打开进程管道。
		 */
		static std::string Exec(const char* cmd);

	};
}

