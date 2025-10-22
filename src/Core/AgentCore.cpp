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
		std::cout << "[AgentCore] ====== STARTING MULTI-TURN CONVERSATION LOOP ======" << std::endl;
		
		// 初始化：创建空的工具执行结果存储
		std::vector<nlohmann::json> toolResults;
		bool continueLoop = true;
		int turnCount = 0;
		
		try
		{
			// 启动循环 (do-while)
			do
			{
				turnCount++;
				std::cout << "[AgentCore] ====== TURN " << turnCount << " ======" << std::endl;
				
				// 1. 构建 Python 命令
				PythonScriptCommand processTurnCommand;
				processTurnCommand.SubCommand = "process_turn";
				processTurnCommand.Args = {{"--tool_definitions_path", m_ToolDefsFilePath}};
				
				if (turnCount == 1)
				{
					// 首次循环：传递音频文件路径
					processTurnCommand.Args.push_back({"--user_input_audio_path", audioFilePath});
					std::cout << "[AgentCore] First turn: sending audio file" << std::endl;
				}
				else
				{
					// 后续循环：传递工具执行结果
					nlohmann::json toolResultsJson = nlohmann::json::array();
					for (const auto& result : toolResults)
					{
						toolResultsJson.push_back(result);
					}
					std::string toolResultsStr = toolResultsJson.dump();
					processTurnCommand.Args.push_back({"--tool_results_json", toolResultsStr});
					std::cout << "[AgentCore] Subsequent turn: sending tool results" << std::endl;
				}
				
				// 2. 执行命令并获取 Python 输出
				std::string command = m_CommandBuilder.BuildCommand(processTurnCommand);
				std::cout << "[AgentCore] Executing process_turn command..." << std::endl;
				std::string output = ProcessUtils::Exec(command.c_str());
				std::cout << "[AgentCore] Received output from Python: " << output << std::endl;
				
				// 3. 提取并解析 JSON
				std::string jsonOutput = ExtractJsonFromOutput(output);
				std::cout << "[AgentCore] Extracted JSON: " << jsonOutput << std::endl;
				
				nlohmann::json responseJson;
				try {
					responseJson = nlohmann::json::parse(jsonOutput, nullptr, false);
					if (responseJson.is_discarded()) {
						throw nlohmann::json::parse_error::create(101, 0, "JSON parsing failed", nullptr);
					}

					if (responseJson.is_string()) {
						const std::string inner = responseJson.get<std::string>();
						auto innerJson = nlohmann::json::parse(inner, nullptr, false);
						if (innerJson.is_discarded() || !innerJson.is_object()) {
							throw nlohmann::json::parse_error::create(101, 0, "Nested JSON parsing failed", nullptr);
						}
						responseJson = std::move(innerJson);
					}

					if (!responseJson.is_object()) {
						throw nlohmann::json::parse_error::create(101, 0, "Top-level is not JSON object", nullptr);
					}
				}
				catch (const nlohmann::json::parse_error& e) {
					std::cerr << "[AgentCore] JSON Parse Error: " << e.what() << std::endl;
					std::cerr << "[AgentCore] Raw output: " << output << std::endl;
					std::cerr << "[AgentCore] Extracted JSON: " << jsonOutput << std::endl;
					
					// 尝试从原始输出中提取final_response
					if (output.find("final_response") != std::string::npos) {
						// 简单的文本提取作为备用方案
						size_t start = output.find("\"final_response\":");
						if (start != std::string::npos) {
							start = output.find("\"", start + 17); // 找到值的开始引号
							if (start != std::string::npos) {
								size_t end = start + 1;
								int escapeCount = 0;
								// 寻找结束引号，考虑转义字符
								while (end < output.length()) {
									if (output[end] == '\\') {
										escapeCount++;
									} else if (output[end] == '"' && escapeCount % 2 == 0) {
										break;
									} else {
										escapeCount = 0;
									}
									end++;
								}
								if (end < output.length()) {
									std::string extractedText = output.substr(start + 1, end - start - 1);
									std::cout << "[AgentCore] Extracted final response as fallback: " << extractedText << std::endl;
									GenerateAndSpeakResponse(extractedText);
									continueLoop = false;
									continue;
								}
							}
						}
					}
					
					// 如果无法提取，播报错误
					GenerateAndSpeakResponse("抱歉，处理过程中遇到了数据格式错误。");
					continueLoop = false;
					continue;
				}
				
				// 4. 解析 Python 返回的 JSON
				if (responseJson.contains("tool_calls"))
				{
					// 情况A：模型需要调用工具
					std::cout << "[AgentCore] Model requests tool execution" << std::endl;
					
					// 清空上一轮的工具结果
					toolResults.clear();
					
					// 遍历并执行所有工具调用
					nlohmann::json toolCalls = responseJson["tool_calls"];
					for (const auto& toolCall : toolCalls)
					{
						std::string toolName = toolCall["name"];
						nlohmann::json toolArgs = toolCall["args"];
						
						std::cout << "[AgentCore] Executing tool: " << toolName << std::endl;
						
						// 通过 ToolRegistry 执行工具
						std::string toolResult = ToolRegistry::GetInstance().ExecuteTool(toolName, toolArgs);
						std::cout << "[AgentCore] Tool '" << toolName << "' result: " << toolResult << std::endl;
						
						// 将工具名和执行结果存入 toolResults
						nlohmann::json resultEntry;
						resultEntry["tool_name"] = toolName;
						resultEntry["content"] = toolResult;
						toolResults.push_back(resultEntry);
					}
					
					// 设置循环继续
					continueLoop = true;
				}
				else if (responseJson.contains("final_response"))
				{
					// 情况B：模型认为任务完成
					std::string finalResponse = responseJson["final_response"];
					std::cout << "[AgentCore] Model provided final response: " << finalResponse << std::endl;
					
					// 调用 TTS 播报最终回复
					GenerateAndSpeakResponse(finalResponse);
					
					// 设置循环结束
					continueLoop = false;
				}
				else if (responseJson.contains("error"))
				{
					// 情况C：发生错误
					std::string errorMsg = responseJson["error"];
					std::string errorDetails = responseJson.value("details", "No details provided");
					std::cerr << "[AgentCore] Error from Python: " << errorMsg << std::endl;
					std::cerr << "[AgentCore] Error details: " << errorDetails << std::endl;
					
					// 播报错误信息
					GenerateAndSpeakResponse("抱歉，处理您的请求时遇到了问题：" + errorMsg);
					
					// 设置循环结束
					continueLoop = false;
				}
				else
				{
					// 未知响应格式
					std::cerr << "[AgentCore] Unknown response format from Python" << std::endl;
					GenerateAndSpeakResponse("抱歉，我无法理解系统的响应格式。");
					continueLoop = false;
				}
				
				// 防止无限循环的安全检查
				if (turnCount >= 10)
				{
					std::cerr << "[AgentCore] Warning: Maximum turn count reached, stopping loop" << std::endl;
					GenerateAndSpeakResponse("抱歉，任务处理时间过长，已自动停止。");
					continueLoop = false;
				}
				
			} while (continueLoop);
			
			std::cout << "[AgentCore] Multi-turn conversation completed in " << turnCount << " turns" << std::endl;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "[AgentCore] JSON Parse Error: " << e.what() << std::endl;
			GenerateAndSpeakResponse("抱歉，处理过程中遇到了数据格式错误。");
			m_CurrentState = AgentState::Idle;
		}
		catch (const std::runtime_error& e)
		{
			std::cerr << "[AgentCore] Runtime Error: " << e.what() << std::endl;
			GenerateAndSpeakResponse("抱歉，处理过程中遇到了系统错误。");
			m_CurrentState = AgentState::Idle;
		}
		catch (const std::exception& e)
		{
			std::cerr << "[AgentCore] Unexpected Error: " << e.what() << std::endl;
			GenerateAndSpeakResponse("抱歉，遇到了意外错误。");
			m_CurrentState = AgentState::Idle;
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

	void AgentCore::GenerateAndSpeakResponse(const std::string& finalResponseText)
	{
		std::cout << "[AgentCore] ====== TTS AND AUDIO PLAYBACK ======" << std::endl;

		try
		{
			// 直接使用传入的最终回复文本进行TTS
			m_CurrentState = AgentState::Speaking;
			PythonScriptCommand ttsCommand;
			ttsCommand.SubCommand = "tts";
			ttsCommand.Args = {
				{"--text", finalResponseText},
				{"--output_file", m_OutputAudioPath}
			};

			std::string command = m_CommandBuilder.BuildCommand(ttsCommand);
			std::cout << "[AgentCore] Executing TTS command..." << std::endl;
			std::string ttsOutput = ProcessUtils::Exec(command.c_str());
			std::cout << "[AgentCore] Received TTS output: " << ttsOutput << std::endl;

			// 解析TTS返回的JSON
			try {
				std::string ttsJsonOutput = ExtractJsonFromOutput(ttsOutput);
				nlohmann::json ttsResult = nlohmann::json::parse(ttsJsonOutput);
				
				if (ttsResult.contains("error")) {
					std::cerr << "[AgentCore] TTS Error: " << ttsResult["error"].get<std::string>() << std::endl;
					if (ttsResult.contains("details")) {
						std::cerr << "[AgentCore] TTS Error Details: " << ttsResult["details"].get<std::string>() << std::endl;
					}
				} else if (ttsResult.contains("status") && ttsResult["status"] == "success") {
					std::cout << "[AgentCore] TTS Success: " << ttsResult["message"].get<std::string>() << std::endl;
				}
			}
			catch (const nlohmann::json::parse_error& e) {
				// 如果解析失败，忽略错误，继续播放音频
				std::cerr << "[AgentCore] Warning: Failed to parse TTS JSON output: " << e.what() << std::endl;
			}

			std::cout << "[AgentCore] Playing response audio..." << std::endl;
			m_AudioManager->PlayAudioFile(m_OutputAudioPath);

			// 播报完成后，返回 Idle 状态
			m_CurrentState = AgentState::Idle;
			std::cout << "[AgentCore] Audio playback finished. Switched back to Idle state." << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cerr << "[AgentCore] Error in TTS/audio playback: " << e.what() << std::endl;
			m_CurrentState = AgentState::Idle;
		}
	}

	std::string AgentCore::ExtractJsonFromOutput(const std::string& output)
	{
		// 从输出中提取JSON部分，使用更健壮的方法
		size_t firstBrace = output.find('{');
		if (firstBrace == std::string::npos) {
			// 如果没有找到JSON开始标记，返回整个输出
			return output;
		}

		// 从第一个 { 开始，寻找匹配的 }
		int braceCount = 0;
		size_t jsonEnd = firstBrace;
		
		for (size_t i = firstBrace; i < output.length(); ++i) {
			if (output[i] == '{') {
				braceCount++;
			}
			else if (output[i] == '}') {
				braceCount--;
				if (braceCount == 0) {
					jsonEnd = i;
					break;
				}
			}
		}

		if (braceCount == 0 && jsonEnd > firstBrace) {
			// 找到了完整的JSON对象
			std::string jsonStr = output.substr(firstBrace, jsonEnd - firstBrace + 1);
			
			// 清理可能的控制字符和确保UTF-8编码正确
			std::string cleanedJson;
			cleanedJson.reserve(jsonStr.length());
			
			for (size_t i = 0; i < jsonStr.length(); ++i) {
				unsigned char c = static_cast<unsigned char>(jsonStr[i]);
				
				// 保留可打印ASCII字符、UTF-8字符和JSON必要的控制字符
				if (c >= 32 || c == '\n' || c == '\r' || c == '\t') {
					cleanedJson += jsonStr[i];
				}
				// 跳过其他控制字符
			}
			// 替换反引号为双引号，修复JSON语法错误
			std::string finalJson = cleanedJson;
			size_t pos = 0;
			while ((pos = finalJson.find('`', pos)) != std::string::npos) {
				finalJson.replace(pos, 1, "\"");
				pos += 1; // 避免无限循环
			}

			return finalJson;
		}
		else {
			// 如果没有找到完整的JSON结构，返回整个输出
			return output;
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