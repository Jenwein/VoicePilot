#pragma once

#include <string>
namespace Razel {
	class ProcessUtils
	{
	public:
		static std::string Exec(const char* cmd);
	};
}

