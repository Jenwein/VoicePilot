#include "AgentCore.h"

#include <iostream>
#include <string>

#include "../Tools/ToolRegistry.h"	
#include "../Tools/SystemTools.h"

namespace Razel
{
	AgentCore::AgentCore()
		: m_CurrentState(AgentState::Idle)
	{
		m_AudioManager = CreateScope<AudioManager>();
		RegisterAllTools();
		SaveToolDefinitionsToFile();
	}

	AgentCore::~AgentCore()	
	{
	}

	void AgentCore::ToggleRecordingAndProcess()
	{
		if (m_CurrentState == AgentState::Idle)
		{
			m_CurrentState = AgentState::Listening;
			m_AudioManager->StartRecording(m_InputAudioPath);
			std::cout << "[AgentCore] Switched to Listening state." << std::endl;
		}
		else if (m_CurrentState == AgentState::Listening)
		{
			m_AudioManager->StopRecording();
			std::cout << "[AgentCore] Recording stopped." << std::endl;

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
		// 1. 语音转文本
		// pycmd -ASR得到文本/错误json

		// 2. 工具调用处理
		// pycmd -LLM 得到规定好的FunctionCalling格式/错误json

		// 3. 响应生成
		// GenerateAndSpeakResponse(resultText)
	}

	void AgentCore::GenerateAndSpeakResponse(const std::string& resultText)
	{
		// 1. 使用TTS生成语音文件
		// pycmd -TTS 生成语音文件在assets/audios/output.wav

		// 2. 播放生成的语音文件
		// m_AudioManager->PlayAudio(OutAudioPath);
	}

	void AgentCore::RegisterAllTools()
	{
		std::cout << "[AgentCore] Registering all tools..." << std::endl;
		auto& registry = ToolRegistry::GetInstance();

		registry.RegisterTool<GetCurrentTimeTool>("get_current_time");
		registry.RegisterTool<WriteFileTool>("write_to_file");
		registry.RegisterTool<GetKnownFolderPathTool>("get_known_folder_path");
	}

	void AgentCore::SaveToolDefinitionsToFile()
	{
		try
		{
			nlohmann::json allToolDefs = ToolRegistry::GetInstance().GetAllToolDefinitions();
			std::ofstream file(m_ToolDefsFilePath);
			if (file.is_open())
			{
				file << allToolDefs.dump(4);
				file.close();
				std::cout << "[AgentCore] Tool definitions saved to " << m_ToolDefsFilePath << std::endl;
			}
			else
			{
				std::cerr << "[AgentCore] Error: Could not open file " << m_ToolDefsFilePath << " for writing." << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "[AgentCore] Error saving tool definitions: " << e.what() << std::endl;
		}
	}	

}