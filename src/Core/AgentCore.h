#pragma once
#include <Razel.h>
#include "../Audio/AudioManager.h"
#include "../Core/ScriptCommandBuilder.h"
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
	private:
		void RegisterAllTools();
		void GenerateAndSpeakResponse(const std::string& finalResponseText);
		void SaveToolDefinitionsToFile();
		std::string ExtractJsonFromOutput(const std::string& output);
	private:
		AgentState m_CurrentState;
		Scope<AudioManager> m_AudioManager;
		ScriptCommandBuilder m_CommandBuilder;
		const std::string m_InputAudioPath = "Resources/audios/input.wav";
		const std::string m_OutputAudioPath = "Resources/audios/output.wav";
		const std::string m_ToolDefsFilePath = "Resources/prompts/toolDefsPrompt.json";
	};
}
