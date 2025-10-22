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
		std::filesystem::path scriptPath = m_ScriptsPath / "ai_service.py";
		std::stringstream cmd;

		cmd << "\"" << m_PythonExecutablePath.string() << "\"";
		cmd << " \"" << scriptPath.string() << "\"";
		cmd << " " << commandInfo.SubCommand;

		for (const auto& arg : commandInfo.Args) {
			cmd << " " << arg.Flag << " ";

			std::string escapedValue = arg.Value;
#ifdef _WIN32
			size_t pos = 0;
			while ((pos = escapedValue.find("\"", pos)) != std::string::npos) {
				escapedValue.replace(pos, 1, "\\\"");
				pos += 2;
			}
#endif

			cmd << "\"" << escapedValue << "\"";
		}

		return cmd.str();
	}
}