#include "AIServiceWapper.h"
#include "PythonManager.h"

#include <iostream>

namespace Razel
{
	AIServiceWrapper::AIServiceWrapper()
	{
	}

	AIServiceWrapper::~AIServiceWrapper()
	{
	}

	bool AIServiceWrapper::Initialize()
	{
		if (m_Initialized)
		{
			return true;
		}

		try
		{
			// 确保PythonManager已初始化
			auto& pythonManager = PythonManager::GetInstance();
			if (!pythonManager.IsInitialized())
			{
				if (!pythonManager.Initialize())
				{
					SetLastError("Failed to initialize Python Manager");
					return false;
				}
			}

			// 导入ai_service模块
			m_AIServiceModule = pythonManager.ImportModule("ai_service");

			std::cout << "[AIServiceWrapper] AI Service module imported successfully." << std::endl;
			m_Initialized = true;
			return true;
		}
		catch (const std::exception& e)
		{
			SetLastError("Failed to initialize AI Service: " + std::string(e.what()));
			std::cerr << "[AIServiceWrapper] " << m_LastError << std::endl;
			return false;
		}
	}

	Razel::AIResult AIServiceWrapper::TranscribeAudio(const std::string& audioFilePath /*= ""*/)
	{
		if (!m_Initialized)
		{
			AIResult result;
			result.success = false;
			result.error_type = "Initialization Error";
			result.error_details = "AIServiceWrapper not initialized";
			return result;
		}

		try
		{
			nlohmann::json args;
			if (!audioFilePath.empty())
			{
				args["audio_file_path"] = audioFilePath;
			}

			return CallPythonFunction("transcribe_audio", args);
		}
		catch (const std::exception& e)
		{
			AIResult result;
			result.success = false;
			result.error_type = "ASR Error";
			result.error_details = e.what();
			SetLastError("ASR failed: " + std::string(e.what()));
			return result;
		}
	}

	Razel::AIResult AIServiceWrapper::ProcessUserRequest(const std::string& userRequest, const std::string& toolsFile, const std::string& previousTurn /*= ""*/)
	{
		if (!m_Initialized)
		{
			AIResult result;
			result.success = false;
			result.error_type = "Initialization Error";
			result.error_details = "AIServiceWrapper not initialized";
			return result;
		}

		try
		{
			nlohmann::json args;
			args["user_request"] = userRequest;
			args["tools_file"] = toolsFile;
			if (!previousTurn.empty())
			{
				args["previous_turn"] = previousTurn;
			}

			return CallPythonFunction("process_user_request", args);
		}
		catch (const std::exception& e)
		{
			AIResult result;
			result.success = false;
			result.error_type = "LLM Error";
			result.error_details = e.what();
			SetLastError("LLM processing failed: " + std::string(e.what()));
			return result;
		}
	}

	Razel::AIResult AIServiceWrapper::SynthesizeSpeech(const std::string& text, const std::string& outputFilePath /*= ""*/)
	{
		if (!m_Initialized)
		{
			AIResult result;
			result.success = false;
			result.error_type = "Initialization Error";
			result.error_details = "AIServiceWrapper not initialized";
			return result;
		}

		try
		{
			nlohmann::json args;
			args["text"] = text;
			if (!outputFilePath.empty())
			{
				args["output_file_path"] = outputFilePath;
			}

			return CallPythonFunction("synthesize_speech", args);
		}
		catch (const std::exception& e)
		{
			AIResult result;
			result.success = false;
			result.error_type = "TTS Error";
			result.error_details = e.what();
			SetLastError("TTS failed: " + std::string(e.what()));
			return result;
		}
	}

	Razel::AIResult AIServiceWrapper::CallPythonFunction(const std::string& functionName, const nlohmann::json& args)
	{
		try
		{
			py::object pyFunction = m_AIServiceModule.attr(functionName.c_str());
			py::object pyResult;

			// 根据参数调用Python函数
			if (args.empty())
			{
				pyResult = pyFunction();
			}
			else
			{
				py::dict pyArgs;
				for (auto& [key, value] : args.items())
				{
					if (value.is_string())
					{
						pyArgs[key.c_str()] = value.get<std::string>();
					}
					else if (value.is_number_integer())
					{
						pyArgs[key.c_str()] = value.get<int>();
					}
					else if (value.is_number_float())
					{
						pyArgs[key.c_str()] = value.get<double>();
					}
					else if (value.is_boolean())
					{
						pyArgs[key.c_str()] = value.get<bool>();
					}
					// 其他类型可以根据需要扩展
				}

				// 使用关键字参数调用
				pyResult = pyFunction(**pyArgs);
			}

			return ConvertPythonResult(pyResult);
		}
		catch (const py::error_already_set& e)
		{
			AIResult result;
			result.success = false;
			result.error_type = "Python Error";
			result.error_details = e.what();
			SetLastError("Python function call failed: " + std::string(e.what()));
			return result;
		}
		catch (const std::exception& e)
		{
			AIResult result;
			result.success = false;
			result.error_type = "Function Call Error";
			result.error_details = e.what();
			SetLastError("Function call failed: " + std::string(e.what()));
			return result;
		}
	}

	Razel::AIResult AIServiceWrapper::ConvertPythonResult(const pybind11::object& pyResult)
	{
		AIResult result;

		try
		{
			// 将Python字典转换为C++可用的格式
			py::dict pyDict = pyResult.cast<py::dict>();

			// 检查是否成功
			if (pyDict.contains("status"))
			{
				std::string status = pyDict["status"].cast<std::string>();
				result.success = (status == "success" || status == "continue" || status == "finished");
			}
			else if (pyDict.contains("error"))
			{
				result.success = false;
				result.error_type = pyDict["error"].cast<std::string>();

				if (pyDict.contains("details"))
				{
					result.error_details = pyDict["details"].cast<std::string>();
				}
			}
			else
			{
				// 如果没有明确的状态或错误字段，假设成功
				result.success = true;
			}

			// 转换整个字典为JSON
			std::string jsonStr = py::str(pyResult).cast<std::string>();

			// 尝试解析为JSON（需要处理Python字典格式到JSON格式的转换）
			try
			{
				py::module json_module = py::module::import("json");
				py::object jsonObj = json_module.attr("dumps")(pyResult, py::arg("ensure_ascii") = false);
				std::string jsonString = jsonObj.cast<std::string>();
				result.data = nlohmann::json::parse(jsonString);
			}
			catch (...)
			{
				// 如果JSON转换失败，手动处理关键字段，确保UTF-8编码
				if (pyDict.contains("transcript"))
				{
					// 确保transcript字段的UTF-8编码正确处理
					py::str transcriptStr = pyDict["transcript"].cast<py::str>();
					std::string transcript = transcriptStr.cast<std::string>();
					result.data["transcript"] = transcript;
				}
				if (pyDict.contains("function_calls"))
				{
					// 处理function_calls（复杂数据结构）
					result.data["function_calls"] = "complex_data";
				}
				if (pyDict.contains("response_text"))
				{
					py::str responseStr = pyDict["response_text"].cast<py::str>();
					std::string responseText = responseStr.cast<std::string>();
					result.data["response_text"] = responseText;
				}
				if (pyDict.contains("message"))
				{
					py::str messageStr = pyDict["message"].cast<py::str>();
					std::string message = messageStr.cast<std::string>();
					result.data["message"] = message;
				}
			}

		}
		catch (const std::exception& e)
		{
			result.success = false;
			result.error_type = "Conversion Error";
			result.error_details = "Failed to convert Python result: " + std::string(e.what());
			SetLastError(result.error_details);
		}

		return result;
	}

	void AIServiceWrapper::SetLastError(const std::string& error)
	{
		m_LastError = error;
		std::cerr << "[AIServiceWrapper] Error: " << error << std::endl;
	}

}
