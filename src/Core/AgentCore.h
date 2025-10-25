#pragma once
#include <Razel.h>
#include <functional>
#include <future>
#include <mutex>
#include "VoiceProcessingPipeline.h"
#include "../Audio/AudioManager.h"
#include "../Python/AIServiceWrapper.h"

namespace Razel
{
    enum class AgentState
    {
        Idle,
        Listening,
        Processing,
        Speaking
    };

    // 配置结构体
    struct VoiceAssistantConfig
    {
        std::string inputAudioPath = "Resources/audios/input.wav";
        std::string outputAudioPath = "Resources/audios/output.wav";
        std::string toolDefsPath = "Resources/prompts/toolDefsPrompt.json";
        int maxProcessingTimeoutSeconds = 30;
    };

    class AgentCore
    {
    public:
        AgentCore();
        AgentCore(const VoiceAssistantConfig& config);
        ~AgentCore();

        void OnUpdate();

        void StartListening();  // 开始录音
        void StopListening();   // 停止录音，自动开始处理
        void StartSpeaking(const std::string& filePath);
        void CancelOperation(); // 取消当前操作
        
        AgentState GetCurrentState() const;
        bool CanStartNewSession() const;
        
        void SetStateChangeCallback(std::function<void(AgentState, AgentState)> callback);

        // 异步处理
        void ProcessVoiceRequestAsync();
    private:
        void OnProcessingComplete(const PipelineResult& result);
        void OnPlaybackFinished();
        void ChangeState(AgentState newState);
        bool CanTransitionTo(AgentState newState) const;

        void HandleError(const std::string& errorMessage);

        void OnPipelineStageChanged(PipelineStage stage, const std::string& message);
        void SaveToolDefinitionsToFile();

    private:
        // 线程安全的状态管理
        mutable std::mutex m_StateMutex;
        AgentState m_CurrentState;

        VoiceAssistantConfig m_Config;
        
        // 回调保护
        mutable std::mutex m_CallbackMutex;
        std::function<void(AgentState, AgentState)> m_StateChangeCallback;

        // 异步任务管理
        std::future<PipelineResult> m_ProcessingTask;

        Scope<AudioManager> m_AudioManager;
        Scope<AIServiceWrapper> m_AIServiceWrapper;
        Scope<VoiceProcessingPipeline> m_Pipeline;
    };
}
