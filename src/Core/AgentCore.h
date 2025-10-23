#pragma once
#include <Razel.h>
#include "../Audio/AudioManager.h"
#include "../Python/AIServiceWapper.h"

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

		void ToggleRecordingAndProcess();
		AgentState GetCurrentState() const { return m_CurrentState; }
		void OnUpdate();
		void ProcessAudio(const std::string& audioFilePath);//TMP:PUBILC
		bool CanStartNewConversation() const { return m_CurrentState == AgentState::Idle; }
	private:
		void RegisterAllTools();
		void GenerateAndSpeakResponse(const std::string& toolResult);
		void SaveToolDefinitionsToFile();

		bool ProcessUserRequestWithTools(const std::string& userRequest, std::string& finalResponse);
		bool ExecuteToolCalls(const nlohmann::json& functionCalls, std::string& toolResults);
		std::string ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters);

	private:
		AgentState m_CurrentState;
		Scope<AudioManager> m_AudioManager;
		Scope<AIServiceWrapper> m_AIServiceWrapper;

		const std::string m_InputAudioPath = "Resources/audios/input.wav";
		const std::string m_OutputAudioPath = "Resources/audios/output.wav";
		const std::string m_ToolDefsFilePath = "Resources/prompts/toolDefsPrompt.json";
	
		// 对话历史管理
		std::string m_ConversationHistory;
	};
}
