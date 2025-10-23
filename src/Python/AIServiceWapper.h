#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace Razel
{
	class PythonManager;

	struct AIResult
	{
		bool success = false;
		std::string error_type;
		std::string error_details;
		nlohmann::json data;

		// 便捷方法
		bool IsSuccess() const { return success; }
		std::string GetErrorMessage() const
		{
			if (success) return "";
			return error_type + ": " + error_details;
		}
	};

	class AIServiceWrapper
	{
	public:
		AIServiceWrapper();
		~AIServiceWrapper();

		// 初始化AI服务
		bool Initialize();

		// 语音转文本 (ASR)
		AIResult TranscribeAudio(const std::string& audioFilePath = "");

		// 处理用户请求 (LLM)
		AIResult ProcessUserRequest(const std::string& userRequest,
			const std::string& toolsFile,
			const std::string& previousTurn = "");

		// 文本转语音 (TTS)
		AIResult SynthesizeSpeech(const std::string& text,
			const std::string& outputFilePath = "");

		// 获取最后的错误信息
		std::string GetLastError() const { return m_LastError; }

		// 检查是否已初始化
		bool IsInitialized() const { return m_Initialized; }

	private:
		// 调用Python函数的通用方法
		AIResult CallPythonFunction(const std::string& functionName,
			const nlohmann::json& args);

		// 将Python字典转换为AIResult
		AIResult ConvertPythonResult(const pybind11::object& pyResult);

		// 错误处理
		void SetLastError(const std::string& error);

	private:
		bool m_Initialized = false;
		std::string m_LastError;
		pybind11::module m_AIServiceModule;
	};
}