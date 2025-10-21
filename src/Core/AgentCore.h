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
	private:
		void ProcessAudio(const std::string& audioFilePath);
		void RegisterAllTools();
		void GenerateAndSpeakResponse(const std::string& toolResult);
	private:
		AgentState m_CurrentState;
		Scope<AudioManager> m_AudioManager;
		ScriptCommandBuilder m_CommandBuilder;
		const std::string m_InputAudioPath = "input.wav";	// 定义临时音频文件名
		const std::string m_OutputAudioPath = "output.wav";	// 临时输出音频文件名
	};
}
