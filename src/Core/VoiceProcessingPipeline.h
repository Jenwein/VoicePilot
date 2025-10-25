#pragma once
#include <Razel.h>
#include <functional>
#include <atomic>
#include <mutex>
#include <future>
#include <nlohmann/json.hpp>
#include "../Python/AIServiceWrapper.h"

namespace Razel
{
    struct PipelineResult
    {
        bool success;
        std::string responseText;
		std::string outputFilePath;
        std::string errorMessage;

        PipelineResult(bool s = false, const std::string& response = "", const std::string& path = "", const std::string& error = "")
            : success(s), responseText(response), outputFilePath(path), errorMessage(error) { }

        static PipelineResult Success(const std::string& response)
        {
            return PipelineResult(true, response);
        }

        static PipelineResult Error(const std::string& error)
        {
            return PipelineResult(false, "", error);
        }
    };

    // 流程状态回调
    enum class PipelineStage
    {
        ASR,            // 语音转文本
        LLM,            // 大语言模型处理
        ToolExecution,  // 工具执行
        TTS,            // 文本转语音
    };

    class VoiceProcessingPipeline
    {
    public:
        VoiceProcessingPipeline();
        ~VoiceProcessingPipeline();

        // 同步接口（保留用于向后兼容）
        //PipelineResult ProcessAudioFile(const std::string& inputPath, const std::string& outputPath, const std::string& toolDefsPath);

        // 异步接口
        std::future<PipelineResult> ProcessAudioFileAsync(const std::string& inputPath, const std::string& outputPath, const std::string& toolDefsPath);

        // 设置流程阶段回调
        void SetStageCallback(std::function<void(PipelineStage, const std::string&)> callback);

        // 取消当前处理（线程安全）
        void Cancel();
        bool IsCancelled() const { return m_Cancelled.load(); }

        // 检查是否正在处理
        bool IsProcessing() const { return m_Processing.load(); }

    private:
        // 内部处理方法（现在线程安全）
        PipelineResult ProcessAudioFileInternal(const std::string& inputPath, const std::string& outputPath, const std::string& toolDefsPath);

        // 流程步骤
        PipelineResult PerformASR(const std::string& audioFilePath);
        PipelineResult ProcessWithLLM(const std::string& userRequest, const std::string& toolDefsPath);
        PipelineResult GenerateTTS(const std::string& responseText, const std::string& outputPath);
        //PipelineResult PlayAudio(const std::string& audioPath);

        // LLM处理的内部方法
        bool ProcessUserRequestWithChat(const std::string& userRequest, const std::string& toolDefsPath, std::string& finalResponse);
        bool ExecuteToolCalls(const nlohmann::json& functionCalls, nlohmann::json& toolResults);
        std::string ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters);

        // 工具管理
        void RegisterAllTools();

        // 状态管理（线程安全）
        void NotifyStage(PipelineStage stage, const std::string& message);
        bool CheckCancellation();  // 检查是否需要取消

    private:
        Scope<AIServiceWrapper> m_AIServiceWrapper;

        std::unique_ptr<PythonCILRelease> m_GILRelease;
        // 线程安全的状态管理
        std::atomic<bool> m_Cancelled;
        std::atomic<bool> m_Processing;
        std::atomic<bool> m_ChatSessionActive;

        // 回调保护
        mutable std::mutex m_CallbackMutex;
        std::function<void(PipelineStage, const std::string&)> m_StageCallback;
    };
}