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

		/**
		* @brief 切换录音状态并处理音频。
		* 如果当前是 Idle 状态, 则开始录音。
		* 如果当前是 Listening 状态, 则停止录音并开始处理。
		*/
		void ToggleRecordingAndProcess();

		AgentState GetCurrentState() const { return m_CurrentState; }
		void OnUpdate();
	private:
		/**
		 * @brief 处理录制好的音频文件。
		 * 调用 Python 脚本进行“理解与规划”。
		 */
		void ProcessAudio(const std::string& audioFilePath);
	private:
		AgentState m_CurrentState;
		Scope<AudioManager> m_AudioManager;
		ScriptCommandBuilder m_CommandBuilder;
		const std::string m_InputAudioPath = "input.wav"; // 定义临时音频文件名
	};
}
