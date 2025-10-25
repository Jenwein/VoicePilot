#pragma once

#include <memory>
#include <string>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <iostream>
namespace py = pybind11;
namespace Razel
{
	class PythonCILRelease
	{
	public:
		PythonCILRelease()
			:m_Release(std::make_unique<py::gil_scoped_release>())
		{
			std::cout << "[PythonManager] Main thread Releasing  GIL..." << std::endl;
			
		}
		~PythonCILRelease()
		{
			std::cout << "[PythonManager] Re-acquiring main thread GIL..." << std::endl;
		}

		PythonCILRelease(const PythonCILRelease&) = delete;
		PythonCILRelease& operator=(const PythonCILRelease&) = delete;
		PythonCILRelease(PythonCILRelease&&) = delete;
		PythonCILRelease& operator=(PythonCILRelease&&) = delete;
	private:
		std::unique_ptr<py::gil_scoped_release> m_Release;
	};
	class PythonCILAcquire
	{
	public:
		PythonCILAcquire()
			:m_acquire (std::make_unique<py::gil_scoped_acquire>())
		{
			std::cout << "[PythonManager] Acquire thread GIL..." << std::endl;
		}
		~PythonCILAcquire()
		{
			std::cout << "[PythonManager] Acquire thread GIL..." << std::endl;

		}

		PythonCILAcquire(const PythonCILAcquire&) = delete;
		PythonCILAcquire& operator=(const PythonCILAcquire&) = delete;
		PythonCILAcquire(PythonCILAcquire&&) = delete;
		PythonCILAcquire& operator=(PythonCILAcquire&&) = delete;
	private:
		std::unique_ptr<py::gil_scoped_acquire> m_acquire;
	};

	class PythonManager
	{
	public:
		// 获取单例实例
		static PythonManager& GetInstance();

		// 初始化 Python 环境
		bool Initialize();

		// 清理 Python 环境
		void Shutdown();

		// 检查是否已初始化
		bool IsInitialized() const { return m_Initialized; }

		// 导入模块
		py::module ImportModule(const std::string& moduleName);

		// 添加 Python 路径
		void AddPythonPath(const std::string& path);

		// 执行 Python 代码（用于调试）
		bool ExecuteCode(const std::string& code);

		// 获取 Python 版本信息
		std::string GetPythonVersion() const;

		// 错误处理 - 获取最后的 Python 异常信息
		std::string GetLastError() const;

	private:
		PythonManager() = default;
		~PythonManager();

		// 禁止复制和移动
		PythonManager(const PythonManager&) = delete;
		PythonManager& operator=(const PythonManager&) = delete;
		PythonManager(PythonManager&&) = delete;
		PythonManager& operator=(PythonManager&&) = delete;

		// 设置 Python 路径和环境
		void SetupPythonEnvironment();

	private:
		bool m_Initialized = false;
		std::unique_ptr<py::scoped_interpreter> m_Interpreter;
		std::string m_LastError;
	};
}
