#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <map>

namespace Razel {

	struct CommandArg {
		std::string Flag;
		std::string Value;
	};

	struct PythonScriptCommand {
		std::string SubCommand;
		std::vector<CommandArg> Args;
	};


	class ScriptCommandBuilder
	{
	public:
		ScriptCommandBuilder();

		std::string BuildCommand(const PythonScriptCommand& commandInfo);

	private:
		std::filesystem::path m_ScriptsPath;
		std::filesystem::path m_PythonExecutablePath;
	};

}