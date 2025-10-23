#include "PythonManager.h"
#include <iostream>
#include <filesystem>
namespace Razel
{
	Razel::PythonManager& PythonManager::GetInstance()
	{
		static PythonManager instance;
		return instance;
	}

	PythonManager::~PythonManager()
	{
		Shutdown();
	}

	bool PythonManager::Initialize()
	{
		if (m_Initialized)
		{
			return true;
		}

		try
		{
			// 初始化 Python 解释器
			m_Interpreter = std::make_unique<py::scoped_interpreter>();

			// 设置 Python 环境
			m_Initialized = true;
			SetupPythonEnvironment();

			std::cout << "Python Manager initialized successfully." << std::endl;
			std::cout << "Python version: " << GetPythonVersion() << std::endl;

			return true;
		}
		catch (const std::exception& e)
		{
			m_LastError = "Failed to initialize Python: " + std::string(e.what());
			std::cerr << m_LastError << std::endl;
			return false;
		}
	}

	void PythonManager::Shutdown()
	{
		if (m_Initialized)
		{
			try
			{
				m_Interpreter.reset();
				m_Initialized = false;
				std::cout << "Python Manager shutdown successfully." << std::endl;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error during Python shutdown: " << e.what() << std::endl;
			}
		}
	}

	py::module PythonManager::ImportModule(const std::string& moduleName)
	{
		if (!m_Initialized)
		{
			throw std::runtime_error("Python Manager not initialized");
		}

		try
		{
			return py::module::import(moduleName.c_str());
		}
		catch (const py::error_already_set& e)
		{
			m_LastError = "Failed to import module '" + moduleName + "': " + e.what();
			throw std::runtime_error(m_LastError);
		}
	}

	void PythonManager::AddPythonPath(const std::string& path)
	{
		if (!m_Initialized)
		{
			throw std::runtime_error("Python Manager not initialized");
		}

		try
		{
			py::module sys = py::module::import("sys");
			py::list sys_path = sys.attr("path");

			// 检查路径是否已存在
			bool pathExists = false;
			for (const auto& existingPath : sys_path)
			{
				if (existingPath.cast<std::string>() == path)
				{
					pathExists = true;
					break;
				}
			}

			if (!pathExists)
			{
				sys_path.insert(0, path);
				std::cout << "Added Python path: " << path << std::endl;
			}
		}
		catch (const py::error_already_set& e)
		{
			m_LastError = "Failed to add Python path '" + path + "': " + e.what();
			throw std::runtime_error(m_LastError);
		}
	}

	bool PythonManager::ExecuteCode(const std::string& code)
	{
		if (!m_Initialized)
		{
			m_LastError = "Python Manager not initialized";
			return false;
		}

		try
		{
			py::exec(code);
			return true;
		}
		catch (const py::error_already_set& e)
		{
			m_LastError = "Failed to execute Python code: " + std::string(e.what());
			return false;
		}
	}

	std::string PythonManager::GetPythonVersion() const
	{
		if (!m_Initialized)
		{
			return "Python not initialized";
		}

		try
		{
			py::module sys = py::module::import("sys");
			py::tuple version = sys.attr("version_info");

			int major = version[0].cast<int>();
			int minor = version[1].cast<int>();
			int micro = version[2].cast<int>();

			return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(micro);
		}
		catch (const std::exception&)
		{
			return "Unknown";
		}
	}

	std::string PythonManager::GetLastError() const
	{
		return m_LastError;
	}

	void PythonManager::SetupPythonEnvironment()
	{
		try
		{
			// 添加当前工作目录到 Python 路径
			std::filesystem::path currentPath = std::filesystem::current_path();
			AddPythonPath(currentPath.string());

			// 添加 scripts 目录到 Python 路径（假设 ai_service.py 在这里）
			std::filesystem::path scriptsPath = currentPath / "scripts";
			if (std::filesystem::exists(scriptsPath))
			{
				AddPythonPath(scriptsPath.string());
			}

			// 设置Python编码为UTF-8（Windows兼容性）
			ExecuteCode("import sys; import os; import locale");
			ExecuteCode("import codecs");

			// 设置环境变量
			ExecuteCode("os.environ['PYTHONIOENCODING'] = 'utf-8'");
			ExecuteCode("os.environ['LANG'] = 'en_US.UTF-8'");

			// 设置标准输出编码
			ExecuteCode("sys.stdout.reconfigure(encoding='utf-8')");
			ExecuteCode("sys.stderr.reconfigure(encoding='utf-8')");

			// 验证重要的模块是否可用
			try
			{
				py::module::import("json");
				py::module::import("os");
				std::cout << "Core Python modules verified." << std::endl;
			}
			catch (const py::error_already_set& e)
			{
				throw std::runtime_error("Core Python modules not available: " + std::string(e.what()));
			}

			try
			{
				py::module::import("google.genai");
				std::cout << "Google GenAI module available." << std::endl;
			}
			catch (const py::error_already_set&)
			{
				std::cout << "Warning: Google GenAI module not available. Make sure it's installed." << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Failed to setup Python environment: " + std::string(e.what()));
		}
	}
	
}