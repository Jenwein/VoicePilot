#include "ScriptCommandBuilder.h"
#include <sstream>
#include <iostream>

namespace Razel {

	ScriptCommandBuilder::ScriptCommandBuilder()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path scriptsPath;
		for (int i = 0; i < 5; ++i) {
			if (std::filesystem::exists(currentPath / "scripts")) {
				scriptsPath = currentPath / "scripts";
				break;
			}
			if (currentPath.has_parent_path()) {
				currentPath = currentPath.parent_path();
			}
			else {
				break;
			}
		}
		if (scriptsPath.empty()) {
			throw std::runtime_error("Could not find 'scripts' directory relative to the executable path.");
		}
		m_ScriptsPath = scriptsPath;
		std::cout << "[ScriptCommandBuilder] Found scripts directory at: " << m_ScriptsPath << std::endl;

		// TODO:路径可能有更好的处理方式
#ifdef _WIN32
		m_PythonExecutablePath = m_ScriptsPath / ".venv" / "Scripts" / "python.exe";
#else
		m_PythonExecutablePath = m_ScriptsPath / ".venv" / "bin" / "python";
#endif

		if (!std::filesystem::exists(m_PythonExecutablePath)) {
			throw std::runtime_error("Python executable not found in venv at: " + m_PythonExecutablePath.string());
		}
		std::cout << "[ScriptCommandBuilder] Found Python executable at: " << m_PythonExecutablePath << std::endl;
	}


	std::string ScriptCommandBuilder::BuildCommand(const PythonScriptCommand& commandInfo)
	{
		// TODO:当前的拼接无法正常运行
		std::filesystem::path scriptPath = m_ScriptsPath / "ai_service.py";
		std::stringstream cmd;

		cmd << "\"" << m_PythonExecutablePath.string() << "\" "
			<< "\"" << scriptPath.string() << "\" ";

		cmd << commandInfo.SubCommand;

		for (const auto& arg : commandInfo.Args) {
			cmd << " " << arg.Flag << " ";
			if (arg.Value.find(' ') != std::string::npos) {
				cmd << "\"" << arg.Value << "\"";
			}
			else {
				if (arg.Flag == "--file_path") {
					cmd << "\"" << std::filesystem::absolute(arg.Value).string() << "\"";
				}
				else {
					cmd << "\"" << arg.Value << "\"";
				}
			}
		}

		return cmd.str();
	}
}