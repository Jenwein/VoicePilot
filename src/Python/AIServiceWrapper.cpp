#include "AIServiceWrapper.h"

#include <iostream>

namespace Razel
{
    AIServiceWrapper::AIServiceWrapper()
    {
    }

    AIServiceWrapper::~AIServiceWrapper()
	{
        //if (m_ChatSessionActive)
        //{
        //    DestroyChatSession();
        //}

        //m_AIServiceModule = py::module();
        DestroyChatSession();
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
            pythonManager.ImportModule("ai_service");

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

    // === ASR功能 (保持不变) ===
    AIResult AIServiceWrapper::TranscribeAudio(const std::string& audioFilePath)
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
            std::cout << "[AIServiceWrapper] === ASR CALL DEBUG ===" << std::endl;
            std::cout << "[AIServiceWrapper] Audio file path: " << audioFilePath << std::endl;
            
            nlohmann::json args;
            if (!audioFilePath.empty())
            {
                args["audio_file_path"] = audioFilePath;
            }
            
            std::cout << "[AIServiceWrapper] ASR args: " << args.dump(2) << std::endl;
            std::cout << "[AIServiceWrapper] ==========================" << std::endl;

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

    bool AIServiceWrapper::CreateChatSession(const std::string& toolsFile)
    {
        if (!m_Initialized)
        {
            SetLastError("AIServiceWrapper not initialized");
            return false;
        }

        //if (m_ChatSessionActive)
        //{
        //    // 先销毁现有会话
        //    DestroyChatSession();
        //}

        try
        {
            std::cout << "[AIServiceWrapper] === CREATE CHAT SESSION DEBUG ===" << std::endl;
            std::cout << "[AIServiceWrapper] Tools file: " << toolsFile << std::endl;
            
            nlohmann::json args;
            args["tools_file"] = toolsFile;
            
            std::cout << "[AIServiceWrapper] Create session args: " << args.dump(2) << std::endl;

            // 调用Python创建Chat会话            
            auto result = PythonManager::GetInstance().
                CallPythonFunction<std::string>("create_chat_session", toolsFile);
            
            if (result == "success")
            {
                //m_ChatSessionActive = true;
                std::cout << "[AIServiceWrapper] Chat session created successfully." << std::endl;
                std::cout << "[AIServiceWrapper] ======================================" << std::endl;
                return true;
            }
            else
            {
                SetLastError("Chat session creation failed: " + result);
                std::cout << "[AIServiceWrapper] Chat session creation failed: " << result << std::endl;
                std::cout << "[AIServiceWrapper] ======================================" << std::endl;
                return false;
            }
        }
        catch (const std::exception& e)
        {
            SetLastError("Chat session creation error: " + std::string(e.what()));
            std::cout << "[AIServiceWrapper] Chat session creation error: " << e.what() << std::endl;
            std::cout << "[AIServiceWrapper] ======================================" << std::endl;
            return false;
        }
    }

    AIResult AIServiceWrapper::ProcessUserMessage(const std::string& userMessage)
    {
        if (!m_Initialized)
        {
            AIResult result;
            result.success = false;
            result.error_type = "Initialization Error";
            result.error_details = "AIServiceWrapper not initialized";
            return result;
        }

        //if (!m_ChatSessionActive)
        //{
        //    AIResult result;
        //    result.success = false;
        //    result.error_type = "Chat Session Error";
        //    result.error_details = "Chat session not active";
        //    return result;
        //}

        try
        {
            std::cout << "[AIServiceWrapper] === PROCESS USER MESSAGE DEBUG ===" << std::endl;
            std::cout << "[AIServiceWrapper] User message: \"" << userMessage << "\"" << std::endl;
            
            nlohmann::json args;
            args["user_message"] = userMessage;
            
            std::cout << "[AIServiceWrapper] Process message args: " << args.dump(2) << std::endl;
            std::cout << "[AIServiceWrapper] ===============================================" << std::endl;

            return CallPythonFunction("process_user_message", args);
        }
        catch (const std::exception& e)
        {
            AIResult result;
            result.success = false;
            result.error_type = "User Message Error";
            result.error_details = e.what();
            SetLastError("User message processing failed: " + std::string(e.what()));
            return result;
        }
    }

    AIResult AIServiceWrapper::SendToolResults(const nlohmann::json& toolResults)
    {
        if (!m_Initialized)
        {
            AIResult result;
            result.success = false;
            result.error_type = "Initialization Error";
            result.error_details = "AIServiceWrapper not initialized";
            return result;
        }

        //if (!m_ChatSessionActive)
        //{
        //    AIResult result;
        //    result.success = false;
        //    result.error_type = "Chat Session Error";
        //    result.error_details = "Chat session not active";
        //    return result;
        //}

        try
        {
            std::cout << "[AIServiceWrapper] === SEND TOOL RESULTS DEBUG ===" << std::endl;
            std::cout << "[AIServiceWrapper] Tool results: " << toolResults.dump(2) << std::endl;
            
            // 将工具结果转换为JSON字符串
            std::string toolResultsJson = toolResults.dump();
            
            nlohmann::json args;
            args["tool_results_json"] = toolResultsJson;
            
            std::cout << "[AIServiceWrapper] Send tool results args: " << args.dump(2) << std::endl;
            std::cout << "[AIServiceWrapper] ==============================================" << std::endl;

            return CallPythonFunction("send_tool_results", args);
        }
        catch (const std::exception& e)
        {
            AIResult result;
            result.success = false;
            result.error_type = "Tool Results Error";
            result.error_details = e.what();
            SetLastError("Tool results sending failed: " + std::string(e.what()));
            return result;
        }
    }

    bool AIServiceWrapper::DestroyChatSession()
    {
        if (!m_Initialized)
        {
            SetLastError("AIServiceWrapper not initialized");
            return false;
        }

        try
        {
            std::cout << "[AIServiceWrapper] === DESTROY CHAT SESSION DEBUG ===" << std::endl;
            
			auto result = PythonManager::GetInstance().
				CallPythonFunction<std::string>("destroy_chat_session");

            if (result == "success")
            {
                std::cout << "[AIServiceWrapper] Chat session destroyed successfully." << std::endl;
                std::cout << "[AIServiceWrapper] ========================================" << std::endl;
                return true;
            }
            else
            {
                SetLastError("Chat session destruction failed: " + result);
                std::cout << "[AIServiceWrapper] Chat session destruction failed: " << result << std::endl;
                std::cout << "[AIServiceWrapper] ========================================" << std::endl;
                return false;
            }
        }
        catch (const std::exception& e)
        {
            SetLastError("Chat session destruction error: " + std::string(e.what()));
            std::cout << "[AIServiceWrapper] Chat session destruction error: " << e.what() << std::endl;
            std::cout << "[AIServiceWrapper] ========================================" << std::endl;
            return false;
        }
    }

    // === TTS功能 (保持不变) ===
    AIResult AIServiceWrapper::SynthesizeSpeech(const std::string& text, 
                                                  const std::string& outputFilePath)
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
            std::cout << "[AIServiceWrapper] === TTS CALL DEBUG ===" << std::endl;
            std::cout << "[AIServiceWrapper] Text to synthesize: \"" << text << "\"" << std::endl;
            std::cout << "[AIServiceWrapper] Output file path: " << outputFilePath << std::endl;
            
            nlohmann::json args;
            args["text"] = text;
            if (!outputFilePath.empty())
            {
                args["output_file_path"] = outputFilePath;
            }

            std::cout << "[AIServiceWrapper] TTS args: " << args.dump(2) << std::endl;
            std::cout << "[AIServiceWrapper] ==========================" << std::endl;

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

    AIResult AIServiceWrapper::CallPythonFunction(const std::string& functionName, const nlohmann::json& args)
    {
		PythonGILAcquire pythonScope;

		AIResult result;
		try
		{
			std::cout << "[AIServiceWrapper] === AI FUNCTION CALL DEBUG ===" << std::endl;
			std::cout << "[AIServiceWrapper] Function: " << functionName << std::endl;
			std::cout << "[AIServiceWrapper] Arguments: " << args.dump(2) << std::endl;
			std::cout << "[AIServiceWrapper] ========================================" << std::endl;

			nlohmann::json jsonData = PythonManager::GetInstance().CallPythonFunction(functionName, args);
			result.data = jsonData;

			// 从返回的json中解析出AIResult的状态
			if (jsonData.contains("status"))
			{
				std::string status = jsonData["status"].get<std::string>();
				result.success = (status == "success" || status == "continue" || status == "finished");
			}
			else if (jsonData.contains("error"))
			{
				result.success = false;
				result.error_type = jsonData["error"].get<std::string>();

				if (jsonData.contains("details"))
				{
					result.error_details = jsonData["details"].get<std::string>();
				}
			}
			else
			{
				// 如果没有明确的状态或错误字段，假设成功
				result.success = true;
			}

			std::cout << "[AIServiceWrapper] === AI FUNCTION RESULT DEBUG ===" << std::endl;
			std::cout << "[AIServiceWrapper] Success: " << (result.IsSuccess() ? "true" : "false") << std::endl;
			if (!result.IsSuccess())
			{
				std::cout << "[AIServiceWrapper] Error: " << result.GetErrorMessage() << std::endl;
			}
			std::cout << "[AIServiceWrapper] Result Data: " << result.data.dump(2) << std::endl;
			std::cout << "[AIServiceWrapper] ===========================================" << std::endl;

			return result;
		}
		catch (const std::exception& e)
		{
			result.success = false;
			result.error_type = "Function Call Error";
			result.error_details = e.what();
			SetLastError("Function call failed: " + std::string(e.what()));

			std::cout << "[AIServiceWrapper] === AI FUNCTION ERROR DEBUG ===" << std::endl;
			std::cout << "[AIServiceWrapper] Function: " << functionName << std::endl;
			std::cout << "[AIServiceWrapper] C++ Error: " << e.what() << std::endl;
			std::cout << "[AIServiceWrapper] ==========================================" << std::endl;

			return result;
		}
    }

    void AIServiceWrapper::SetLastError(const std::string& error)
    {
        m_LastError = error;
        std::cerr << "[AIServiceWrapper] Error: " << error << std::endl;
    }
}
