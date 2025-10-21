#include "AgentCore.h"
#include "../Platform/ProcessUtils.h"
#include "ScriptCommandBuilder.h"

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
		std::cout << "[AgentCore] ====== STAGE 1 & 2: UNDERSTAND AND EXECUTE ======" << std::endl;
		try
		{
			// --- STAGE 1: 理解与规划 ---
			PythonScriptCommand understandCommand;
			understandCommand.SubCommand = "understand";
			understandCommand.Args = {
				{"--file_path", audioFilePath},
				{"--prompt_text", m_ToolDefsFilePath}
			};

			// 3. 构建并执行命令
			std::string command = m_CommandBuilder.BuildCommand(understandCommand);
			std::cout << "[AgentCore] Executing 'understand' command:\n" << command<< std::endl;
			std::string output = ProcessUtils::Exec(command.c_str());
			std::cout << "[AgentCore] Received JSON from Python: " << output << std::endl;

			std::string jsonOutput;
			size_t firstBrace = output.find('{');
			size_t lastBrace = output.rfind('}');

			if (firstBrace != std::string::npos && lastBrace != std::string::npos && firstBrace < lastBrace) {
				jsonOutput = output.substr(firstBrace, lastBrace - firstBrace + 1);
			}
			else {
				jsonOutput = output;
			}

			std::cout << "[AgentCore] Extracted JSON: " << jsonOutput << std::endl;
			
			// --- STAGE 2: 执行 ---

			// 4. 解析 Python 返回的 Function Call JSON
			nlohmann::json functionCallJson = nlohmann::json::parse(jsonOutput);

			if (functionCallJson.contains("error")) {
				throw std::runtime_error("Python script returned an error: " + functionCallJson["error"].get<std::string>());
			}

			std::string toolName = functionCallJson["functionCall"]["name"];
			nlohmann::json toolArgs = functionCallJson["functionCall"]["args"];

			// 5. 调用 ToolRegistry 执行工具
			std::cout << "[AgentCore] Executing tool '" << toolName << "' via ToolRegistry." << std::endl;
			std::string toolResult = ToolRegistry::GetInstance().ExecuteTool(toolName, toolArgs);
			std::cout << "[AgentCore] Tool execution result: " << toolResult << std::endl;

			// 6. 进入下一个阶段：生成并播报回复
			GenerateAndSpeakResponse(toolResult);

		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "[AgentCore] Error: Failed to parse JSON from Python script. Details: " << e.what() << std::endl;
			m_CurrentState = AgentState::Idle;
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "[AgentCore] Error during processing: " << e.what() << std::endl;
			m_CurrentState = AgentState::Idle; // 发生错误时也要返回 Idle 状态
		}
	}

	void AgentCore::RegisterAllTools()
	{
		std::cout << "[AgentCore] Registering all tools..." << std::endl;
		auto& registry = ToolRegistry::GetInstance();

		registry.RegisterTool<GetCurrentTimeTool>("get_current_time");
		registry.RegisterTool<WriteFileTool>("write_to_file");
		registry.RegisterTool<GetKnownFolderPathTool>("get_known_folder_path");
	}

	void AgentCore::GenerateAndSpeakResponse(const std::string& toolResult)
	{
		std::cout << "[AgentCore] ====== STAGE 3 & 4: RESPOND AND SPEAK ======" << std::endl;

		try
		{
			// --- STAGE 3: 响应生成 ---
			PythonScriptCommand genResponseCommand;
			genResponseCommand.SubCommand = "generate_response";
			genResponseCommand.Args = { {"--result_text", toolResult} };

			std::string command = m_CommandBuilder.BuildCommand(genResponseCommand);
			std::cout << "[AgentCore] Executing 'generate_response' command..." << std::endl;
			std::string output = ProcessUtils::Exec(command.c_str());
			std::cout << "[AgentCore] Received output from Python: " << output << std::endl;

			std::string jsonOutput;
			size_t firstBrace = output.find('{');
			size_t lastBrace = output.rfind('}');

			if (firstBrace != std::string::npos && lastBrace != std::string::npos && firstBrace < lastBrace) {
				jsonOutput = output.substr(firstBrace, lastBrace - firstBrace + 1);
			}
			else {
				// 如果没有找到有效的JSON结构，使用整个输出
				jsonOutput = output;
			}

			std::cout << "[AgentCore] Extracted JSON: " << jsonOutput << std::endl;

			std::string finalResponseText = jsonOutput;

			// --- STAGE 4: 播报 ---
			m_CurrentState = AgentState::Speaking;
			PythonScriptCommand ttsCommand;
			ttsCommand.SubCommand = "tts";
			ttsCommand.Args = {
				{"--text", finalResponseText},
				{"--output_file", m_OutputAudioPath}
			};

			command = m_CommandBuilder.BuildCommand(ttsCommand);
			std::cout << "[AgentCore] Executing 'tts' command..." << std::endl;
			ProcessUtils::Exec(command.c_str()); // 执行TTS，不需要捕获输出

			std::cout << "[AgentCore] Playing response audio..." << std::endl;
			m_AudioManager->PlayAudioFile(m_OutputAudioPath);

			// 播报完成后，返回 Idle 状态
			m_CurrentState = AgentState::Idle;
			std::cout << "[AgentCore] Processing finished. Switched back to Idle state." << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "[AgentCore] Error in generation/speaking stage: " << e.what() << std::endl;
			m_CurrentState = AgentState::Idle;
		}
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