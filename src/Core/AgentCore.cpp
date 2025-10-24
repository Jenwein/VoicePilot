#include "AgentCore.h"

#include <iostream>
#include <string>

#include "../Tools/ToolRegistry.h"	
#include "../Tools/SystemTools.h"

namespace Razel
{
    AgentCore::AgentCore()
        : m_CurrentState(AgentState::Idle), m_ChatSessionActive(false)
    {
        // 设置Windows控制台UTF-8支持
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 设置C++流的UTF-8支持
        std::ios_base::sync_with_stdio(false);
        std::wcout.imbue(std::locale(""));

        m_AudioManager = CreateScope<AudioManager>();
        m_AIServiceWrapper = CreateScope<AIServiceWrapper>();

        if (!m_AIServiceWrapper->Initialize())
        {
            std::cerr << "[AgentCore] Failed to initialize AI Service: "
                << m_AIServiceWrapper->GetLastError() << std::endl;
        }
        else
        {
            std::cout << "[AgentCore] AI Service initialized successfully." << std::endl;
        }

        RegisterAllTools();
        SaveToolDefinitionsToFile();
    }

    AgentCore::~AgentCore()
    {
        // 确保销毁Chat会话
        if (m_ChatSessionActive && m_AIServiceWrapper)
        {
            m_AIServiceWrapper->DestroyChatSession();
        }
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
        else
        {
            // 如果当前正在Processing或Speaking状态，忽略切换请求
            std::cout << "[AgentCore] Cannot toggle recording: Agent is busy (current state: "
                << static_cast<int>(m_CurrentState) << ")." << std::endl;
        }
    }

    void AgentCore::OnUpdate()
    {
        // 未来可以根据 m_CurrentState 在这里做一些每帧更新的操作
        // 例如，在 Listening 状态下检测音量等
    }

    void AgentCore::ProcessAudio(const std::string& audioFilePath)
    {
        std::cout << "[AgentCore] Starting audio processing..." << std::endl;

        // 1. 语音转文本
        std::cout << "[AgentCore] Step 1: Transcribing audio..." << std::endl;

        if (!m_AIServiceWrapper->IsInitialized())
        {
            std::cerr << "[AgentCore] AI Service not initialized, cannot process audio." << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        AIResult asrResult = m_AIServiceWrapper->TranscribeAudio(audioFilePath);

        if (!asrResult.IsSuccess())
        {
            std::cerr << "[AgentCore] ASR failed: " << asrResult.GetErrorMessage() << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        // 提取转录文本
        std::string userRequest;
        if (asrResult.data.contains("transcript"))
        {
            userRequest = asrResult.data["transcript"].get<std::string>();
            std::cout << "[AgentCore] Transcription successful: \"" << userRequest << "\"" << std::endl;
        }
        else
        {
            std::cerr << "[AgentCore] ASR result does not contain transcript data." << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        // 检查是否是空的或无效的转录结果
        if (userRequest.empty() || userRequest.length() < 2)
        {
            std::cout << "[AgentCore] Transcription result is too short, ignoring." << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        // 2. 使用Chat处理用户请求
        std::cout << "[AgentCore] Step 2: Processing user request with Chat..." << std::endl;

        std::string finalResponse;
        if (!ProcessUserRequestWithChat(userRequest, finalResponse))
        {
            std::cerr << "[AgentCore] Failed to process user request with Chat." << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        // 3. 响应生成和播放
        std::cout << "[AgentCore] Step 3: Generating and playing speech response..." << std::endl;
        GenerateAndSpeakResponse(finalResponse);

        // 4. 销毁Chat会话（每次对话后清理）
        if (m_ChatSessionActive)
        {
            m_AIServiceWrapper->DestroyChatSession();
            m_ChatSessionActive = false;
            std::cout << "[AgentCore] Chat session destroyed after conversation." << std::endl;
        }

        std::cout << "[AgentCore] Audio processing pipeline completed successfully." << std::endl;
    }

    void AgentCore::GenerateAndSpeakResponse(const std::string& resultText)
    {
        std::cout << "[AgentCore] Generating speech for: \"" << resultText << "\"" << std::endl;

        // 检查响应文本是否为空
        if (resultText.empty())
        {
            std::cout << "[AgentCore] Response text is empty, skipping TTS." << std::endl;
            return;
        }

        // 设置状态为Speaking
        m_CurrentState = AgentState::Speaking;
        std::cout << "[AgentCore] Switched to Speaking state." << std::endl;

        // 1. 使用TTS生成语音文件
        std::cout << "[AgentCore] Step 1: Synthesizing speech..." << std::endl;

        AIResult ttsResult = m_AIServiceWrapper->SynthesizeSpeech(resultText, m_OutputAudioPath);

        if (!ttsResult.IsSuccess())
        {
            std::cerr << "[AgentCore] TTS failed: " << ttsResult.GetErrorMessage() << std::endl;
            m_CurrentState = AgentState::Idle;
            return;
        }

        std::cout << "[AgentCore] Speech synthesis completed successfully." << std::endl;

        // 2. 播放生成的语音文件
        std::cout << "[AgentCore] Step 2: Playing audio response..." << std::endl;

        try
        {
            m_AudioManager->PlayAudioFile(m_OutputAudioPath);
            std::cout << "[AgentCore] Audio playback completed." << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AgentCore] Audio playback failed: " << e.what() << std::endl;
        }

        // 播放完成后恢复到Idle状态
        // 注意：这里假设PlayAudioFile是阻塞的，如果是异步的需要另外处理
        m_CurrentState = AgentState::Idle;
        std::cout << "[AgentCore] Speech response completed, returned to Idle state." << std::endl;
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

    bool AgentCore::ProcessUserRequestWithChat(const std::string& userRequest, std::string& finalResponse)
    {
        const int MAX_ITERATIONS = 10; // 增加最大迭代次数，Chat更高效
        int iteration = 0;

        std::cout << "[AgentCore] === CHAT PROCESSING DEBUG ===" << std::endl;
        std::cout << "[AgentCore] User request: \"" << userRequest << "\"" << std::endl;
        std::cout << "[AgentCore] =================================" << std::endl;

        // 1. 创建新的Chat会话
        if (!m_AIServiceWrapper->CreateChatSession(m_ToolDefsFilePath))
        {
            std::cerr << "[AgentCore] Failed to create Chat session: " << m_AIServiceWrapper->GetLastError() << std::endl;
            return false;
        }
        m_ChatSessionActive = true;
        std::cout << "[AgentCore] Chat session created successfully." << std::endl;

        // 2. 发送用户消息
        AIResult chatResult = m_AIServiceWrapper->ProcessUserMessage(userRequest);
        
        if (!chatResult.IsSuccess())
        {
            std::cerr << "[AgentCore] Failed to process user message: " << chatResult.GetErrorMessage() << std::endl;
            return false;
        }

        // 3. 处理Chat响应循环
        while (iteration < MAX_ITERATIONS)
        {
            iteration++;
            std::cout << "[AgentCore] === CHAT ITERATION " << iteration << " DEBUG ===" << std::endl;

            // 检查Chat状态
            std::string status;
            if (chatResult.data.contains("status"))
            {
                status = chatResult.data["status"].get<std::string>();
            }

            std::cout << "[AgentCore] Chat status: " << status << std::endl;

            if (status == "finished")
            {
                // 对话完成，获取最终响应
                if (chatResult.data.contains("response_text"))
                {
                    finalResponse = chatResult.data["response_text"].get<std::string>();
                    std::cout << "[AgentCore] Final response: \"" << finalResponse << "\"" << std::endl;
                    std::cout << "[AgentCore] =================================" << std::endl;
                    return true;
                }
                else
                {
                    std::cerr << "[AgentCore] Chat finished but no response text found." << std::endl;
                    std::cout << "[AgentCore] =================================" << std::endl;
                    return false;
                }
            }
            else if (status == "continue")
            {
                // 需要执行工具调用
                if (chatResult.data.contains("function_calls"))
                {
                    std::cout << "[AgentCore] Function calls to execute: " << chatResult.data["function_calls"].dump(2) << std::endl;

                    nlohmann::json toolResults;
                    if (!ExecuteToolCalls(chatResult.data["function_calls"], toolResults))
                    {
                        std::cerr << "[AgentCore] Tool execution failed." << std::endl;
                        std::cout << "[AgentCore] =================================" << std::endl;
                        return false;
                    }

                    std::cout << "[AgentCore] Tool execution results: " << toolResults.dump(2) << std::endl;

                    // 发送工具执行结果给Chat
                    chatResult = m_AIServiceWrapper->SendToolResults(toolResults);

                    if (!chatResult.IsSuccess())
                    {
                        std::cerr << "[AgentCore] Failed to send tool results: " << chatResult.GetErrorMessage() << std::endl;
                        std::cout << "[AgentCore] =================================" << std::endl;
                        return false;
                    }

                    std::cout << "[AgentCore] Tool results sent successfully, continuing chat..." << std::endl;
                }
                else
                {
                    std::cerr << "[AgentCore] Chat status is 'continue' but no function calls found." << std::endl;
                    std::cout << "[AgentCore] =================================" << std::endl;
                    return false;
                }
            }
            else
            {
                std::cerr << "[AgentCore] Unknown Chat status: " << status << std::endl;
                std::cout << "[AgentCore] =================================" << std::endl;
                return false;
            }

            std::cout << "[AgentCore] =================================" << std::endl;
        }

        std::cerr << "[AgentCore] Maximum Chat iterations reached, conversation terminated." << std::endl;
        return false;
    }

    bool AgentCore::ExecuteToolCalls(const nlohmann::json& functionCalls, nlohmann::json& toolResults)
    {
        std::cout << "[AgentCore] Executing tool calls..." << std::endl;

        toolResults = nlohmann::json::array();

        try
        {
            // 处理函数调用数组
            for (const auto& call : functionCalls)
            {
                if (!call.contains("name") || !call.contains("args"))
                {
                    std::cerr << "[AgentCore] Invalid function call format." << std::endl;
                    continue;
                }

                std::string toolName = call["name"].get<std::string>();
                nlohmann::json parameters = call["args"];

                std::cout << "[AgentCore] Executing tool: " << toolName << std::endl;

                // 执行单个工具
                std::string result = ExecuteSingleTool(toolName, parameters);

                // 构建结果（Python端期望的格式）
                nlohmann::json callResult;
                callResult["name"] = toolName;  // 工具名称
                callResult["result"] = result;  // 执行结果
                toolResults.push_back(callResult);

                std::cout << "[AgentCore] Tool '" << toolName << "' result: " << result << std::endl;
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AgentCore] Tool execution error: " << e.what() << std::endl;
            
            // 构建错误结果
            nlohmann::json errorResult;
            errorResult["name"] = "error";
            errorResult["result"] = "Tool execution failed: " + std::string(e.what());
            toolResults.push_back(errorResult);
            
            return false;
        }
    }

    std::string AgentCore::ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters)
    {
        try
        {
            auto& registry = ToolRegistry::GetInstance();

            if (!registry.HasTool(toolName))
            {
                return "Error: Tool '" + toolName + "' not found.";
            }

            // 执行工具并获取结果
            std::string result = registry.ExecuteTool(toolName, parameters);

            return result;
        }
        catch (const std::exception& e)
        {
            return "Error executing tool '" + toolName + "': " + std::string(e.what());
        }
    }
}