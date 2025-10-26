#pragma once

#include <memory>
#include <string>
#include <iostream>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
namespace py = pybind11;
namespace Razel
{
	class PythonGILRelease
	{
	public:
		PythonGILRelease()
			:m_Release(std::make_unique<py::gil_scoped_release>())
		{
			std::cout << "[PythonManager] Main thread Releasing  GIL..." << std::endl;
			
		}
		~PythonGILRelease()
		{
			std::cout << "[PythonManager] Re-acquiring main thread GIL..." << std::endl;
		}

		PythonGILRelease(const PythonGILRelease&) = delete;
		PythonGILRelease& operator=(const PythonGILRelease&) = delete;
		PythonGILRelease(PythonGILRelease&&) = delete;
		PythonGILRelease& operator=(PythonGILRelease&&) = delete;
	private:
		std::unique_ptr<py::gil_scoped_release> m_Release;
	};
	class PythonGILAcquire
	{
	public:
		PythonGILAcquire()
			:m_acquire (std::make_unique<py::gil_scoped_acquire>())
		{
			std::cout << "[PythonManager] Acquire thread GIL..." << std::endl;
		}
		~PythonGILAcquire()
		{
			std::cout << "[PythonManager] Releasing thread GIL..." << std::endl;

		}

		PythonGILAcquire(const PythonGILAcquire&) = delete;
		PythonGILAcquire& operator=(const PythonGILAcquire&) = delete;
		PythonGILAcquire(PythonGILAcquire&&) = delete;
		PythonGILAcquire& operator=(PythonGILAcquire&&) = delete;
	private:
		std::unique_ptr<py::gil_scoped_acquire> m_acquire;
	};

	class PythonManager
	{
	public:
		static PythonManager& GetInstance();

		bool Initialize();
		void ImportModule(const std::string& moduleName);
		void AddPythonPath(const std::string& path);
		bool ExecuteCode(const std::string& code);
	
		template<typename R,typename...Args>
		R CallPythonFunction(std::string functionName, Args... args)
		{
			if (!m_Initialized)
			{
				throw std::runtime_error("Python Manager not initialized");
			}
			try
			{
				py::object pyFunction = m_Module.attr(functionName.c_str());
				py::object result = pyFunction(args...);
				return result.cast<R>();
			}
			catch (const py::error_already_set& e)
			{
				m_LastError = "Failed to call function '" + functionName + "': " + e.what();
				throw std::runtime_error(m_LastError);
			}
		}

		nlohmann::json CallPythonFunction(const std::string& functionName, const nlohmann::json& args);

		void Shutdown();

		bool IsInitialized() const { return m_Initialized; }

		std::string GetPythonVersion() const;
		std::string GetLastError() const;

	private:
		PythonManager() = default;
		~PythonManager();

		PythonManager(const PythonManager&) = delete;
		PythonManager& operator=(const PythonManager&) = delete;
		PythonManager(PythonManager&&) = delete;
		PythonManager& operator=(PythonManager&&) = delete;

		void SetupPythonEnvironment();

	private:
		bool m_Initialized = false;
		py::module m_Module;

		std::unique_ptr<py::scoped_interpreter> m_Interpreter;
		std::string m_LastError;
	};
}
