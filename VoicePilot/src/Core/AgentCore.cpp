#include "AgentCore.h"
#include "../Tools/ToolRegistry.h"
#include "../Tools/Tools.h"
#include <iostream>
#include <fstream>

namespace Razel
{
	AgentCore::AgentCore()
		: AgentCore(VoiceAssistantConfig{})
	{
		//if ()
		//{
		//	std::cerr << "[Pipeline] Failed to create Chat session: " << m_AIServiceWrapper->GetLastError() << std::endl;
		//	return false;
		//}

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

		// 创建Pipeline
		m_AIServiceWrapper = CreateRef<AIServiceWrapper>();
		if (!m_AIServiceWrapper->Initialize())
		{
			std::cerr << "[AgentCore] Failed to initialize AI Service: "
				<< m_AIServiceWrapper->GetLastError() << std::endl;
		}
		else
		{
			std::cout << "[AgentCore] AI Service initialized successfully." << std::endl;
		}

		m_Pipeline = CreateScope<VoiceProcessingPipeline>(m_AIServiceWrapper);

		// 设置Pipeline回调（线程安全包装）
		m_Pipeline->SetStageCallback([this](PipelineStage stage, const std::string& message) {
			OnPipelineStageChanged(stage, message);
			});

		SaveToolDefinitionsToFile();

		if (!m_AIServiceWrapper->CreateChatSession(m_Config.toolDefsPath))
		{
			std::cerr << "[Pipeline] Failed to create Chat session: " << m_AIServiceWrapper->GetLastError() << std::endl;
		}
		m_PythonGILRelease = CreateScope<PythonGILRelease>();
		std::cout << "[AgentCore] Initialized successfully." << std::endl;
	}

	AgentCore::~AgentCore()
	{
		CancelOperation();

		if (m_ProcessingTask.valid())
		{
			std::cout << "[AgentCore] Waiting for processing task to finish..." << std::endl;
			m_ProcessingTask.wait(); //  <--- 这是关键的补充！
			std::cout << "[AgentCore] Processing task finished." << std::endl;
		}
	}

	void AgentCore::StartListening()
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);

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
		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			if (m_CurrentState != AgentState::Listening)
			{
				std::cout << "[AgentCore] Cannot stop listening: Not in listening state." << std::endl;
				return;
			}
		}

		m_AudioManager->StopRecording();
		std::cout << "[AgentCore] Stopped listening." << std::endl;

		// 异步开始处理
		ProcessVoiceRequestAsync();
	}

	void AgentCore::StartSpeaking(const std::string& filePath)
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);

		if (!CanTransitionTo(AgentState::Speaking))
		{
			std::cout << "[AgentCore] Cannot start speaking: Invalid state transition from "
				<< static_cast<int>(m_CurrentState) << std::endl;
			return;
		}

		ChangeState(AgentState::Speaking);
		m_AudioManager->PlayAudioFile(filePath, [this]() {
			this->OnPlaybackFinished();
			});
		std::cout << "[AgentCore] Started speaking..." << std::endl;
	}

	void AgentCore::CancelOperation()
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);

		if (m_CurrentState == AgentState::Idle)
		{
			return;
		}

		std::cout << "[AgentCore] canceling current operation..." << std::endl;

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
			// 等待异步任务完成（非阻塞检查）
			if (m_ProcessingTask.valid())
			{
				auto status = m_ProcessingTask.wait_for(std::chrono::milliseconds(0));
				if (status != std::future_status::ready)
				{
					// 任务还在运行，但已经发送取消信号
					std::cout << "[AgentCore] Cancellation signal sent to processing pipeline." << std::endl;
				}
			}
			break;
		}

		ChangeState(AgentState::Idle);
		std::cout << "[AgentCore] Operation cancelled." << std::endl;
	}

	AgentState AgentCore::GetCurrentState() const
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		return m_CurrentState;
	}

	bool AgentCore::CanStartNewSession() const
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		return m_CurrentState == AgentState::Idle;
	}

	void AgentCore::SetStateChangeCallback(std::function<void(AgentState, AgentState)> callback)
	{
		std::lock_guard<std::mutex> lock(m_CallbackMutex);
		m_StateChangeCallback = callback;
	}

	void AgentCore::OnUpdate()
	{
		// 检查异步处理任务是否完成
		if (m_ProcessingTask.valid())
		{
			auto status = m_ProcessingTask.wait_for(std::chrono::milliseconds(0));
			if (status == std::future_status::ready)
			{
				// 任务完成，获取结果
				try
				{
					PipelineResult result = m_ProcessingTask.get();
					OnProcessingComplete(result);
				}
				catch (const std::exception& e)
				{
					HandleError("Processing task exception: " + std::string(e.what()));
				}
			}
		}

		m_AudioManager->OnUpdate();

		// TODO: 添加超时检查
		// 
		// 
		// TODO: 添加其他定期检查逻辑
	}

	void AgentCore::ProcessVoiceRequestAsync()
	{
		std::cout << "[AgentCore] Starting asynchronous voice request processing..." << std::endl;

		{
			std::lock_guard<std::mutex> lock(m_StateMutex);
			ChangeState(AgentState::Processing);
		}

		// 启动异步处理
		m_ProcessingTask = m_Pipeline->ProcessAudioFileAsync(
			m_Config.inputAudioPath,
			m_Config.outputAudioPath,
			m_Config.toolDefsPath
		);
	}

	void AgentCore::OnProcessingComplete(const PipelineResult& result)
	{
		std::cout << "[AgentCore] Processing completed with result: "
			<< (result.success ? "Success" : "Error") << std::endl;

		if (result.success)
		{
			StartSpeaking(m_Config.outputAudioPath);
			std::cout << "[AgentCore] Voice request processing completed successfully." << std::endl;
		}
		else
		{
			HandleError(result.errorMessage);
			return; // HandleError会设置状态为Idle
		}

		// 成功完成，返回Idle状态
		//{
		//    std::lock_guard<std::mutex> lock(m_StateMutex);
		//    ChangeState(AgentState::Idle);
		//}
	}

	void AgentCore::OnPlaybackFinished()
	{
		std::lock_guard<std::mutex> lock(m_StateMutex);
		if (m_CurrentState == AgentState::Speaking)
		{
			ChangeState(AgentState::Idle);
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

		// 线程安全的回调触发
		{
			std::lock_guard<std::mutex> lock(m_CallbackMutex);
			if (m_StateChangeCallback)
			{
				m_StateChangeCallback(oldState, newState);
			}
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
		std::lock_guard<std::mutex> lock(m_StateMutex);
		ChangeState(AgentState::Idle);
	}

	void AgentCore::OnPipelineStageChanged(PipelineStage stage, const std::string& message)
	{
		// 根据Pipeline阶段更新AgentCore状态
		// 注意：这个方法会在Pipeline的后台线程中被调用
		std::lock_guard<std::mutex> lock(m_StateMutex);

		//switch (stage)
		//{
		//    case PipelineStage::ASR:
		//    case PipelineStage::LLM:
		//    case PipelineStage::ToolExecution:
		//    case PipelineStage::TTS:
		if (m_CurrentState != AgentState::Processing)
		{
			ChangeState(AgentState::Processing);
		}
		//       break;
		//}

		// 可以在这里添加更详细的进度信息传递给UI
		std::cout << "[AgentCore] Pipeline: " << message << std::endl;
	}

	void AgentCore::SaveToolDefinitionsToFile()
	{
		try
		{
			std::filesystem::path filePath(m_Config.toolDefsPath);
			if (std::filesystem::exists(filePath))
			{
				std::cout << "[AgentCore] File " << m_Config.toolDefsPath << " already exists. Skipping." << std::endl;
				return;
			}

			if (!filePath.parent_path().empty())
			{
				std::filesystem::create_directories(filePath.parent_path());
			}

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