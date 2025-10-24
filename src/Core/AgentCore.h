#pragma once
#include <Razel.h>
#include "../Audio/AudioManager.h"
#include "../Python/AIServiceWrapper.h"

namespace Razel
{
    enum class AgentState
    {
        Idle, Listening, Processing, Speaking
    };

    class AgentCore
    {
    public:
        AgentCore();
        ~AgentCore();

        void OnUpdate();

        void ToggleRecordingAndProcess(); // 切换录音状态并处理
        
        bool CanStartNewConversation() const { return m_CurrentState == AgentState::Idle; }
        AgentState GetCurrentState() const { return m_CurrentState; }

        void ProcessAudio(const std::string& audioFilePath);//TMP:PUBLIC
    private:
		void RegisterAllTools();                                                // 注册所有工具
        void GenerateAndSpeakResponse(const std::string& toolResult);           // 生成并播放语音回复
        void SaveToolDefinitionsToFile();                                       // 保存工具定义到文件

		bool ProcessUserRequestWithChat(const std::string& userRequest, std::string& finalResponse);    // 处理用户请求与Chat交互
		bool ExecuteToolCalls(const nlohmann::json& functionCalls, nlohmann::json& toolResults);        // 执行工具调用
		std::string ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters);   // 执行单个工具

    private:
        AgentState m_CurrentState;
        Scope<AudioManager> m_AudioManager;
        Scope<AIServiceWrapper> m_AIServiceWrapper;

        const std::string m_InputAudioPath = "Resources/audios/input.wav";
        const std::string m_OutputAudioPath = "Resources/audios/output.wav";
        const std::string m_ToolDefsFilePath = "Resources/prompts/toolDefsPrompt.json";

        // Chat会话状态
        bool m_ChatSessionActive = false;
    };
}
