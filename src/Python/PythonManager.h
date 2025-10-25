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
			std::cout << "[PythonManager] Releasing thread GIL..." << std::endl;

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
