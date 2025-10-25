#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "PythonManager.h"

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

        // === ASR功能 (保持不变) ===
        AIResult TranscribeAudio(const std::string& audioFilePath = "");

        // === LLM功能 (重构为Chat) ===
        // 创建Chat会话
        bool CreateChatSession(const std::string& toolsFile);
        
        // 处理用户消息
        AIResult ProcessUserMessage(const std::string& userMessage);
        
        // 发送工具执行结果
        AIResult SendToolResults(const nlohmann::json& toolResults);
        
        // 销毁Chat会话
        bool DestroyChatSession();

        AIResult SynthesizeSpeech(const std::string& text,
            const std::string& outputFilePath = "");

        // 获取最后的错误信息
        std::string GetLastError() const { return m_LastError; }

        // 检查是否已初始化
        bool IsInitialized() const { return m_Initialized; }

        // 检查Chat会话是否已创建
        bool IsChatSessionActive() const { return m_ChatSessionActive; }

    private:
        // 调用Python函数的通用方法
        AIResult CallPythonFunction(const std::string& functionName,
            const nlohmann::json& args = {});

        // 将Python结果转换为AIResult
        //AIResult ConvertPythonResult(const pybind11::object& pyResult);

        // 设置错误
        void SetLastError(const std::string& error);

    private:
        //std::unique_ptr<PythonCILRelease> m_GILRelease;
        
        bool m_Initialized = false;
        bool m_ChatSessionActive = false;
        std::string m_LastError;
        
    };
}