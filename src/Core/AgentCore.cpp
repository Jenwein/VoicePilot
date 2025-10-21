#include "AgentCore.h"
#include "../Platform/ProcessUtils.h"
#include "ScriptCommandBuilder.h"

#include <iostream>
#include <string>

namespace Razel
{
	AgentCore::AgentCore()
		: m_CurrentState(AgentState::Idle)
	{
		m_AudioManager = CreateScope<AudioManager>();
	}

	AgentCore::~AgentCore()	
	{
	}

	void AgentCore::ToggleRecordingAndProcess()
	{
		if (m_CurrentState == AgentState::Idle)
		{
			// --- 开始录音 ---
			m_CurrentState = AgentState::Listening;
			m_AudioManager->StartRecording(m_InputAudioPath);
			std::cout << "[AgentCore] Switched to Listening state." << std::endl;
		}
		else if (m_CurrentState == AgentState::Listening)
		{
			// --- 停止录音并开始处理 ---
			m_AudioManager->StopRecording();
			std::cout << "[AgentCore] Recording stopped." << std::endl;

			// 切换到处理状态并发起处理流程
			m_CurrentState = AgentState::Processing;
			std::cout << "[AgentCore] Switched to Processing state." << std::endl;
			ProcessAudio(m_InputAudioPath);
		}

	}

	void AgentCore::OnUpdate()
	{
		// 未来可以根据 m_CurrentState 在这里做一些每帧更新的操作
		// 例如，在 Listening 状态下检测音量等
	}

	void AgentCore::ProcessAudio(const std::string& audioFilePath)
	{
		std::cout << "[AgentCore] Processing audio file: " << audioFilePath << std::endl;

		// --- "思考"阶段 ---

		// 1. 填充命令结构体
		Razel::PythonScriptCommand understandCommand;
		understandCommand.SubCommand = "understand";
		understandCommand.Args = {
			{"--file_path", audioFilePath},
			{"--prompt_text", "根据语音内容选择合适的工具并执行。"}
		};

		// 2. 使用通用方法构建命令
		std::string command = m_CommandBuilder.BuildCommand(understandCommand);

		std::cout << "[AgentCore] Executing command: " << command << std::endl;

		try
		{
			// 调用 ProcessUtils::Exec 来执行命令并捕获输出
			std::string jsonOutput = Razel::ProcessUtils::Exec(command.c_str());

			std::cout << "[AgentCore] Received JSON from Python: " << jsonOutput << std::endl;

			// TODO: 下一步将在这里解析 jsonOutput, 并根据结果调用 ToolRegistry 中的工具
			// ...

			// 模拟处理完成，返回 Idle 状态
			m_CurrentState = AgentState::Idle;
			std::cout << "[AgentCore] Processing finished. Switched back to Idle state." << std::endl;
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "[AgentCore] Error executing Python script: " << e.what() << std::endl;
			// 发生错误时也要返回 Idle 状态，以便进行下一次尝试
			m_CurrentState = AgentState::Idle;
		}
	}
}