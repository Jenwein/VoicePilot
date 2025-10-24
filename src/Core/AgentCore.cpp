#include "AgentCore.h"
#include "../Tools/ToolRegistry.h"
#include "../Tools/SystemTools.h"
#include <iostream>
#include <fstream>

namespace Razel
{
    AgentCore::AgentCore()
        : AgentCore(VoiceAssistantConfig{})
    {
    }

    AgentCore::AgentCore(const VoiceAssistantConfig& config)
        : m_CurrentState(AgentState::Idle), 
          m_Config(config)
    {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

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

        // 创建Pipeline
        m_Pipeline = CreateScope<VoiceProcessingPipeline>(m_AudioManager.get(), m_AIServiceWrapper.get());
        
        // 设置Pipeline回调
        m_Pipeline->SetStageCallback([this](PipelineStage stage, const std::string& message) {
            OnPipelineStageChanged(stage, message);
        });

        SaveToolDefinitionsToFile();
        
        std::cout << "[AgentCore] Initialized successfully." << std::endl;
    }

    AgentCore::~AgentCore()
    {
        // 取消任何正在进行的操作
        CancelOperation();
    }

    void AgentCore::StartListening()
    {
        if (!CanTransitionTo(AgentState::Listening))
        {
            std::cout << "[AgentCore] Cannot start listening: Invalid state transition from "
                << static_cast<int>(m_CurrentState) << std::endl;
            return;
        }

        ChangeState(AgentState::Listening);
        m_AudioManager->StartRecording(m_Config.inputAudioPath);
        std::cout << "[AgentCore] Started listening..." << std::endl;
    }

    void AgentCore::StopListening()
    {
        if (m_CurrentState != AgentState::Listening)
        {
            std::cout << "[AgentCore] Cannot stop listening: Not in listening state." << std::endl;
            return;
        }

        m_AudioManager->StopRecording();
        std::cout << "[AgentCore] Stopped listening." << std::endl;

        ChangeState(AgentState::Processing);
        ProcessVoiceRequest();
    }

    void AgentCore::CancelOperation()
    {
        if (m_CurrentState == AgentState::Idle)
        {
            return;
        }

        std::cout << "[AgentCore] Cancelling current operation..." << std::endl;

        switch (m_CurrentState)
        {
            case AgentState::Listening:
                m_AudioManager->StopRecording();
                break;
            case AgentState::Processing:
            case AgentState::Speaking:
                if (m_Pipeline)
                {
                    m_Pipeline->Cancel();
                }
                break;
        }

        ChangeState(AgentState::Idle);
        std::cout << "[AgentCore] Operation cancelled." << std::endl;
    }

    void AgentCore::SetStateChangeCallback(std::function<void(AgentState, AgentState)> callback)
    {
        m_StateChangeCallback = callback;
    }

    void AgentCore::OnUpdate()
    {
        // 未来可以根据 m_CurrentState 在这里做一些每帧更新的操作
        // 例如，在 Listening 状态下检测音量等
        
        // TODO: 添加超时检查
        // TODO: 添加异步操作状态检查
    }

    void AgentCore::ProcessVoiceRequest()
    {
        std::cout << "[AgentCore] Starting voice request processing..." << std::endl;
        
        PipelineResult result = m_Pipeline->ProcessAudioFile(
            m_Config.inputAudioPath, 
            m_Config.outputAudioPath, 
            m_Config.toolDefsPath
        );

        if (result.success)
        {
            std::cout << "[AgentCore] Voice request processing completed successfully." << std::endl;
            ChangeState(AgentState::Idle);
        }
        else
        {
            HandleError(result.errorMessage);
        }
    }

    void AgentCore::ChangeState(AgentState newState)
    {
        if (newState == m_CurrentState)
        {
            return; // 状态没有变化
        }

        AgentState oldState = m_CurrentState;
        m_CurrentState = newState;

        std::cout << "[AgentCore] State changed from " << static_cast<int>(oldState)
                  << " to " << static_cast<int>(newState) << std::endl;

        // 触发回调
        if (m_StateChangeCallback)
        {
            m_StateChangeCallback(oldState, newState);
        }
    }

    bool AgentCore::CanTransitionTo(AgentState newState) const
    {
        switch (m_CurrentState)
        {
            case AgentState::Idle:
                return newState == AgentState::Listening;
            case AgentState::Listening:
                return newState == AgentState::Processing || newState == AgentState::Idle;
            case AgentState::Processing:
                return newState == AgentState::Speaking || newState == AgentState::Idle;
            case AgentState::Speaking:
                return newState == AgentState::Idle;
            default:
                return false;
        }
    }

    void AgentCore::HandleError(const std::string& errorMessage)
    {
        std::cerr << "[AgentCore] Error: " << errorMessage << std::endl;

        // 返回到空闲状态
        ChangeState(AgentState::Idle);
    }

    void AgentCore::OnPipelineStageChanged(PipelineStage stage, const std::string& message)
    {
        // 根据Pipeline阶段更新AgentCore状态
        switch (stage)
        {
            case PipelineStage::ASR:
            case PipelineStage::LLM:
            case PipelineStage::ToolExecution:
            case PipelineStage::TTS:
                if (m_CurrentState != AgentState::Processing)
                {
                    ChangeState(AgentState::Processing);
                }
                break;
                
            case PipelineStage::AudioPlayback:
                if (m_CurrentState != AgentState::Speaking)
                {
                    ChangeState(AgentState::Speaking);
                }
                break;
        }
        
        // 可以在这里添加更详细的进度信息传递给UI
        std::cout << "[AgentCore] Pipeline: " << message << std::endl;
    }

    void AgentCore::SaveToolDefinitionsToFile()
    {
        try
        {
            nlohmann::json allToolDefs = ToolRegistry::GetInstance().GetAllToolDefinitions();
            std::ofstream file(m_Config.toolDefsPath);
            if (file.is_open())
            {
                file << allToolDefs.dump(4);
                file.close();
                std::cout << "[AgentCore] Tool definitions saved to " << m_Config.toolDefsPath << std::endl;
            }
            else
            {
                std::cerr << "[AgentCore] Error: Could not open file " << m_Config.toolDefsPath << " for writing." << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AgentCore] Error saving tool definitions: " << e.what() << std::endl;
        }
    }
}