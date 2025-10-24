#pragma once
#include <Razel.h>
#include <functional>
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
		void CancelOperation(); // 取消当前操作
        
        AgentState GetCurrentState() const { return m_CurrentState; }
        bool CanStartNewSession() const { return m_CurrentState == AgentState::Idle; }
        
        void SetStateChangeCallback(std::function<void(AgentState, AgentState)> callback);

        // 核心处理流程 /TODO:测试,临时public
        void ProcessVoiceRequest();

    private:
		void ChangeState(AgentState newState);
		bool CanTransitionTo(AgentState newState) const;

        void HandleError(const std::string& errorMessage);

        void OnPipelineStageChanged(PipelineStage stage, const std::string& message);
        void SaveToolDefinitionsToFile();
    private:
        AgentState m_CurrentState;
        VoiceAssistantConfig m_Config;
        std::function<void(AgentState, AgentState)> m_StateChangeCallback;

        Scope<AudioManager> m_AudioManager;
        Scope<AIServiceWrapper> m_AIServiceWrapper;
        Scope<VoiceProcessingPipeline> m_Pipeline;
    };
}
