#pragma once
#include <Razel.h>
#include <functional>
#include <nlohmann/json.hpp>
#include "../Audio/AudioManager.h"
#include "../Python/AIServiceWrapper.h"

namespace Razel
{
    struct PipelineResult
    {
        bool success;
        std::string responseText;
        std::string errorMessage;

        PipelineResult(bool s = false, const std::string& response = "", const std::string& error = "")
            : success(s), responseText(response), errorMessage(error) {}

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
        AudioPlayback   // 音频播放
    };

    class VoiceProcessingPipeline
    {
    public:
        VoiceProcessingPipeline(AudioManager* audioManager, AIServiceWrapper* aiService);
        ~VoiceProcessingPipeline();

        PipelineResult ProcessAudioFile(const std::string& inputPath, const std::string& outputPath, const std::string& toolDefsPath);

        // 设置流程阶段回调（用于UI进度显示）
        void SetStageCallback(std::function<void(PipelineStage, const std::string&)> callback);

        // 取消当前处理（用于中断支持）
        void Cancel();
        bool IsCancelled() const { return m_Cancelled; }

    private:
        // 流程步骤
        PipelineResult PerformASR(const std::string& audioFilePath);
        PipelineResult ProcessWithLLM(const std::string& userRequest, const std::string& toolDefsPath);
        PipelineResult GenerateTTS(const std::string& responseText, const std::string& outputPath);
        PipelineResult PlayAudio(const std::string& audioPath);

        // LLM处理的内部方法
        bool ProcessUserRequestWithChat(const std::string& userRequest, const std::string& toolDefsPath, std::string& finalResponse);
        bool ExecuteToolCalls(const nlohmann::json& functionCalls, nlohmann::json& toolResults);
        std::string ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters);

        // 工具管理
        void RegisterAllTools();

        // 状态管理
        void NotifyStage(PipelineStage stage, const std::string& message);
        bool CheckCancellation();  // 检查是否需要取消

    private:
        // 依赖的服务
        AudioManager* m_AudioManager;
        AIServiceWrapper* m_AIServiceWrapper;

        // 回调和状态
        std::function<void(PipelineStage, const std::string&)> m_StageCallback;
        bool m_Cancelled;
        bool m_ChatSessionActive;
    };
}