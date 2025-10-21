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
		std::filesystem::path scriptPath = m_ScriptsPath / "ai_service.py";
		std::stringstream cmd;

		cmd << "\"" << m_PythonExecutablePath.string() << "\"";
		cmd << " \"" << scriptPath.string() << "\"";
		cmd << " " << commandInfo.SubCommand;

		for (const auto& arg : commandInfo.Args) {
			cmd << " " << arg.Flag << " ";
			// 对参数值进行转义处理，特别是处理双引号
			std::string escapedValue = arg.Value;
			// 将双引号替换为转义双引号
			size_t pos = 0;
			while ((pos = escapedValue.find("\"", pos)) != std::string::npos) {
				escapedValue.replace(pos, 1, "\\\"");
				pos += 2; // 移动到替换后的位置
			}
			// 将换行符替换为空格
			pos = 0;
			while ((pos = escapedValue.find("\n", pos)) != std::string::npos) {
				escapedValue.replace(pos, 1, " ");
			}
			// 将回车符替换为空格
			pos = 0;
			while ((pos = escapedValue.find("\r", pos)) != std::string::npos) {
				escapedValue.replace(pos, 1, " ");
			}
			cmd << "\"" << escapedValue << "\"";
		}

		return cmd.str();
	}
}