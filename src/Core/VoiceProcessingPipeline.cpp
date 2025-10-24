#include "VoiceProcessingPipeline.h"
#include "../Tools/ToolRegistry.h"
#include "../Tools/SystemTools.h"
#include <iostream>

namespace Razel
{
    VoiceProcessingPipeline::VoiceProcessingPipeline(AudioManager* audioManager, AIServiceWrapper* aiService)
        : m_AudioManager(audioManager)
        , m_AIServiceWrapper(aiService)
        , m_Cancelled(false)
        , m_ChatSessionActive(false)
    {
        RegisterAllTools();
    }

    VoiceProcessingPipeline::~VoiceProcessingPipeline()
    {
        // 确保清理Chat会话
        if (m_ChatSessionActive && m_AIServiceWrapper)
        {
            m_AIServiceWrapper->DestroyChatSession();
        }
    }

    PipelineResult VoiceProcessingPipeline::ProcessAudioFile(const std::string& inputPath, const std::string& outputPath, const std::string& toolDefsPath)
    {
        std::cout << "[Pipeline] Starting voice processing pipeline..." << std::endl;
        
        m_Cancelled = false;

        try
        {
            // 步骤1: ASR
            auto asrResult = PerformASR(inputPath);
            if (!asrResult.success || CheckCancellation())
            {
                return asrResult;
            }

            // 步骤2: LLM处理
            auto llmResult = ProcessWithLLM(asrResult.responseText, toolDefsPath);
            if (!llmResult.success || CheckCancellation())
            {
                return llmResult;
            }

            // 步骤3: TTS生成
            auto ttsResult = GenerateTTS(llmResult.responseText, outputPath);
            if (!ttsResult.success || CheckCancellation())
            {
                return ttsResult;
            }

            // 步骤4: 音频播放
            auto playResult = PlayAudio(outputPath);
            if (!playResult.success || CheckCancellation())
            {
                return playResult;
            }

            std::cout << "[Pipeline] Voice processing pipeline completed successfully." << std::endl;
            return PipelineResult::Success(llmResult.responseText);
        }
        catch (const std::exception& e)
        {
            std::string errorMsg = "Pipeline processing failed: " + std::string(e.what());
            std::cerr << "[Pipeline] " << errorMsg << std::endl;
            return PipelineResult::Error(errorMsg);
        }
    }

    void VoiceProcessingPipeline::SetStageCallback(std::function<void(PipelineStage, const std::string&)> callback)
    {
        m_StageCallback = callback;
    }

    void VoiceProcessingPipeline::Cancel()
    {
        std::cout << "[Pipeline] Cancelling pipeline processing..." << std::endl;
        m_Cancelled = true;

        if (m_ChatSessionActive && m_AIServiceWrapper)
        {
            m_AIServiceWrapper->DestroyChatSession();
            m_ChatSessionActive = false;
        }
    }

    PipelineResult VoiceProcessingPipeline::PerformASR(const std::string& audioFilePath)
    {
        NotifyStage(PipelineStage::ASR, "Starting speech recognition...");
        
        if (!m_AIServiceWrapper->IsInitialized())
        {
            return PipelineResult::Error("AI Service not initialized");
        }

        AIResult asrResult = m_AIServiceWrapper->TranscribeAudio(audioFilePath);
        if (!asrResult.IsSuccess())
        {
            return PipelineResult::Error("ASR failed: " + asrResult.GetErrorMessage());
        }

        std::string userRequest;
        if (asrResult.data.contains("transcript"))
        {
            userRequest = asrResult.data["transcript"].get<std::string>();
            std::cout << "[Pipeline] ASR successful: \"" << userRequest << "\"" << std::endl;
        }
        else
        {
            return PipelineResult::Error("ASR result does not contain transcript data");
        }

        if (userRequest.empty() || userRequest.length() < 2)
        {
            return PipelineResult::Error("Transcription result is too short or empty");
        }

        NotifyStage(PipelineStage::ASR, "Speech recognition completed: " + userRequest);
        return PipelineResult::Success(userRequest);
    }

    PipelineResult VoiceProcessingPipeline::ProcessWithLLM(const std::string& userRequest, const std::string& toolDefsPath)
    {
        NotifyStage(PipelineStage::LLM, "Starting LLM processing...");
        
        std::string finalResponse;
        if (!ProcessUserRequestWithChat(userRequest, toolDefsPath, finalResponse))
        {
            return PipelineResult::Error("Failed to process user request with LLM");
        }

        NotifyStage(PipelineStage::LLM, "LLM processing completed");
        return PipelineResult::Success(finalResponse);
    }

    PipelineResult VoiceProcessingPipeline::GenerateTTS(const std::string& responseText, const std::string& outputPath)
    {
        NotifyStage(PipelineStage::TTS, "Starting text-to-speech synthesis...");

        if (responseText.empty())
        {
            return PipelineResult::Error("Response text is empty");
        }

        AIResult ttsResult = m_AIServiceWrapper->SynthesizeSpeech(responseText, outputPath);

        if (!ttsResult.IsSuccess())
        {
            return PipelineResult::Error("TTS failed: " + ttsResult.GetErrorMessage());
        }

        NotifyStage(PipelineStage::TTS, "Text-to-speech synthesis completed");
        return PipelineResult::Success(responseText);
    }

    PipelineResult VoiceProcessingPipeline::PlayAudio(const std::string& audioPath)
    {
        NotifyStage(PipelineStage::AudioPlayback, "Starting audio playback...");

        try
        {
            m_AudioManager->PlayAudioFile(audioPath);
            NotifyStage(PipelineStage::AudioPlayback, "Audio playback completed");
            return PipelineResult::Success("");
        }
        catch (const std::exception& e)
        {
            return PipelineResult::Error("Audio playback failed: " + std::string(e.what()));
        }
    }

    bool VoiceProcessingPipeline::ProcessUserRequestWithChat(const std::string& userRequest, const std::string& toolDefsPath, std::string& finalResponse)
    {
        const int MAX_ITERATIONS = 10;
        int iteration = 0;

        std::cout << "[Pipeline] === CHAT PROCESSING ===" << std::endl;
        std::cout << "[Pipeline] User request: \"" << userRequest << "\"" << std::endl;

        // 1. 创建Chat会话
        if (!m_AIServiceWrapper->CreateChatSession(toolDefsPath))
        {
            std::cerr << "[Pipeline] Failed to create Chat session: " << m_AIServiceWrapper->GetLastError() << std::endl;
            return false;
        }
        m_ChatSessionActive = true;

        // 2. 发送用户消息
        AIResult chatResult = m_AIServiceWrapper->ProcessUserMessage(userRequest);
        
        if (!chatResult.IsSuccess())
        {
            std::cerr << "[Pipeline] Failed to process user message: " << chatResult.GetErrorMessage() << std::endl;
            return false;
        }

        // 3. 处理Chat响应循环
        while (iteration < MAX_ITERATIONS && !CheckCancellation())
        {
            iteration++;
            
            std::string status;
            if (chatResult.data.contains("status"))
            {
                status = chatResult.data["status"].get<std::string>();
            }

            if (status == "finished")
            {
                if (chatResult.data.contains("response_text"))
                {
                    finalResponse = chatResult.data["response_text"].get<std::string>();
                    std::cout << "[Pipeline] Final response: \"" << finalResponse << "\"" << std::endl;
                    
                    if (m_ChatSessionActive)
                    {
                        m_AIServiceWrapper->DestroyChatSession();
                        m_ChatSessionActive = false;
                    }
                    
                    return true;
                }
                else
                {
                    std::cerr << "[Pipeline] Chat finished but no response text found." << std::endl;
                    return false;
                }
            }
            else if (status == "continue")
            {
                if (chatResult.data.contains("function_calls"))
                {
                    NotifyStage(PipelineStage::ToolExecution, "Executing tools...");
                    
                    nlohmann::json toolResults;
                    if (!ExecuteToolCalls(chatResult.data["function_calls"], toolResults))
                    {
                        std::cerr << "[Pipeline] Tool execution failed." << std::endl;
                        return false;
                    }

                    // 发送工具结果
                    chatResult = m_AIServiceWrapper->SendToolResults(toolResults);
                    if (!chatResult.IsSuccess())
                    {
                        std::cerr << "[Pipeline] Failed to send tool results: " << chatResult.GetErrorMessage() << std::endl;
                        return false;
                    }
                }
                else
                {
                    std::cerr << "[Pipeline] Chat status is 'continue' but no function calls found." << std::endl;
                    return false;
                }
            }
            else
            {
                std::cerr << "[Pipeline] Unknown Chat status: " << status << std::endl;
                return false;
            }
        }

        if (CheckCancellation())
        {
            std::cout << "[Pipeline] Chat processing cancelled by user." << std::endl;
            return false;
        }

        std::cerr << "[Pipeline] Maximum Chat iterations reached." << std::endl;
        return false;
    }

    bool VoiceProcessingPipeline::ExecuteToolCalls(const nlohmann::json& functionCalls, nlohmann::json& toolResults)
    {
        toolResults = nlohmann::json::array();

        try
        {
            for (const auto& call : functionCalls)
            {
                if (CheckCancellation()) return false;

                if (!call.contains("name") || !call.contains("args"))
                {
                    std::cerr << "[Pipeline] Invalid function call format." << std::endl;
                    continue;
                }

                std::string toolName = call["name"].get<std::string>();
                nlohmann::json parameters = call["args"];

                std::string result = ExecuteSingleTool(toolName, parameters);

                nlohmann::json callResult;
                callResult["name"] = toolName;
                callResult["result"] = result;
                toolResults.push_back(callResult);
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[Pipeline] Tool execution error: " << e.what() << std::endl;
            return false;
        }
    }

    std::string VoiceProcessingPipeline::ExecuteSingleTool(const std::string& toolName, const nlohmann::json& parameters)
    {
        try
        {
            auto& registry = ToolRegistry::GetInstance();

            if (!registry.HasTool(toolName))
            {
                return "Error: Tool '" + toolName + "' not found.";
            }

            return registry.ExecuteTool(toolName, parameters);
        }
        catch (const std::exception& e)
        {
            return "Error executing tool '" + toolName + "': " + std::string(e.what());
        }
    }

    void VoiceProcessingPipeline::RegisterAllTools()
    {
        std::cout << "[Pipeline] Registering all tools..." << std::endl;
        auto& registry = ToolRegistry::GetInstance();

        registry.RegisterTool<GetCurrentTimeTool>("get_current_time");
        registry.RegisterTool<WriteFileTool>("write_to_file");
        registry.RegisterTool<GetKnownFolderPathTool>("get_known_folder_path");
    }

    void VoiceProcessingPipeline::NotifyStage(PipelineStage stage, const std::string& message)
    {
        std::cout << "[Pipeline] " << message << std::endl;
        
        if (m_StageCallback)
        {
            m_StageCallback(stage, message);
        }
    }

    bool VoiceProcessingPipeline::CheckCancellation()
    {
        return m_Cancelled;
    }
}