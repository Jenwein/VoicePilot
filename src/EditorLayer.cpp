#include "EditorLayer.h"
#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Razel/Scene/SceneSerializer.h"
#include "Razel/Utils/PlatformUtils.h"

#include "ImGuizmo.h"
#include "Razel/Math/Math.h"

#include <thread>

namespace Razel {

	extern const std::filesystem::path g_AssetPath = "assets";
	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_CameraController(1280.0f / 720.0f), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f }), m_ViewportFocused(false), m_ViewportHovered(false)
	{
	}

	void EditorLayer::OnAttach()
	{
		RZ_PROFILE_FUNCTION();

		m_CheckerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");
		m_IconPlay = Texture2D::Create("Resources/Icons/PlayButton.png");
		m_IconStop = Texture2D::Create("Resources/Icons/StopButton.png");

		FramebufferSpecification fbSpec;
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		fbSpec.Attachments = {
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::Depth
		};

		m_Framebuffer = Framebuffer::Create(fbSpec);
		m_ActiveScene = CreateRef<Scene>();
		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);

		m_AgentCore = CreateScope<AgentCore>();
		//m_AgentCore->Init();

		// TODO:创建语音助手的3D模型实体
		//m_VoiceAssistantEntity = m_ActiveScene->CreateEntity("VoiceAssistant");
		// ModelComponent modelComp;
		// modelComp.FilePath = "assets/models/bunny/bunny.obj";
		// modelComp.FlipUVs = false;
		// modelComp.Model = CreateRef<Model>(modelComp.FilePath, modelComp.FlipUVs);
		// auto& modelComponent = m_VoiceAssistantEntity.AddComponent<ModelComponent>(modelComp);
		// 
		// auto& transformComponent = m_VoiceAssistantEntity.GetComponent<TransformComponent>();
		// //transformComponent.Translation = { 0.0f, 0.0f, 0.0f };
		// transformComponent.Rotation = { 0.0f, 90.0f, 0.0f };
		// transformComponent.Scale = { 3.0f, 3.0f, 3.0f };
	}

	void EditorLayer::OnDetach()
	{
		RZ_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		RZ_PROFILE_FUNCTION();

		m_AgentCore->OnUpdate();

		//TODO:启用语音助手3D模型的更新,当前的py调用是同步阻塞的,所以更新无法进行,之后考虑异步的进行py的调用与结果获取
		//UpdateVoiceAssistantModel(ts);

		// 当帧缓冲大小与视口大小不同时,且视口大小不为0
		// 因为当前的流程中,先OnUpdate,渲染,填充帧缓冲,解绑,然后在OnImGuiRenderer中去调整视口大小,此时会导致纹理为空,所以有一个黑色的闪烁
		if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
			m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);

			m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);

			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		// Render
		Renderer2D::ResetStats();

		m_Framebuffer->Bind();

		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();


		static float rotation = 0.0f;
		rotation += ts * 50.0f;

		RZ_PROFILE_SCOPE("Renderer Draw");
		//Renderer2D::BeginScene(m_CameraController.GetCamera());

		m_Framebuffer->ClearAttachment(1, -1);

		switch (m_SceneState)
		{
		case Razel::EditorLayer::SceneState::Edit:
		{
			if (m_ViewportFocused)
				m_CameraController.OnUpdate(ts);

			m_EditorCamera.OnUpdate(ts);
			m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
			break;
		}
		case Razel::EditorLayer::SceneState::Play:
		{
			m_ActiveScene->OnUpdateRuntime(ts);
			break;
		}
		default:
			break;
		}

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
			m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
			RZ_CORE_WARN("Pixel data = {0}", pixelData);
		}

		OnOverlayRender();

		m_Framebuffer->UnBind();

	}

	void EditorLayer::OnImGuiRender()
	{
		RZ_PROFILE_FUNCTION();

		// Note: Switch this to true to enable dockspace

		static bool dockspaceOpen = true;						// 停靠空间开启
		static bool opt_fullscreen_persistant = true;			// 全屏持久化
		bool opt_fullscreen = opt_fullscreen_persistant;		// 全屏
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
		{
			ImGui::PopStyleVar(2);
		}

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();

		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		else
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
			ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
			ImGui::SameLine(0.0f, 0.0f);
			if (ImGui::SmallButton("click here"))
				io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		}

		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows,
				// which we can't undo at the moment without finer window depth/z control.

				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					NewScene();
				}
				if (ImGui::MenuItem("Open...", "Ctrl+O"))
				{
					OpenScene();
				}
				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					SaveSceneAs();
				}


				if (ImGui::MenuItem("Exit")) Application::Get().Close();
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// Voice Pilot
		// 显示当前的AI助手的内容及任务(以及开始停止等信息)
		//ImGui::Begin("Content");
		////TODO:
		//ImGui::End();


		//AgentState currentState = m_AgentCore->GetCurrentState();
		//switch (currentState)
		//{
		//}

		ImGui::Begin("TODO");
		if (ImGui::Button("test_ExecPython"))
		{
			m_AgentCore->ProcessAudio("input.wav");
		}
		
		//TODO:
		ImGui::End();

		// 后期来的及可以添加悬浮球的UI，当主界面最小化后显示悬浮球
		//ImGui::Begin("悬浮球");
		////TODO:作为一个悬浮球
		//ImGui::End();


		// TODO:ViewPort显示一个3D模型作为语言助手的形象，后期如果来的及可以通过控制模型的变化来表现当前AI助手的状态
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0,0 });
		ImGui::Begin("Viewport");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);

		// 获取当前 ImGui 窗口或面板中 可用的内容区域的大小
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				OpenScene(std::filesystem::path(g_AssetPath / path));
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::End();
		ImGui::PopStyleVar();

		//UI_ToolBars();

		ImGui::End();
	}

	void EditorLayer::UI_ToolBars()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 2 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{ 0, 0 });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0, 0, 0, 0 });
		auto& colors = ImGui::GetStyle().Colors;
		const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { buttonHovered.x,buttonHovered.y,buttonHovered.z,0.0f });
		const auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { buttonActive.x,buttonActive.y,buttonActive.z,0.0f });

		ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float size = ImGui::GetWindowHeight() - 4.0f;
		Ref<Texture2D> icon = m_SceneState == SceneState::Edit ? m_IconPlay : m_IconStop;
		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - (size * 0.5f));
		if (ImGui::ImageButton("PlayButton", (ImTextureID)icon->GetRendererID(), ImVec2(size, size), { 0,0 }, { 1,1 }))
		{
			if (m_SceneState == SceneState::Edit)
				OnScenePlay();
			else if (m_SceneState == SceneState::Play)
				OnSceneStop();
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_CameraController.OnEvent(e);
		m_EditorCamera.OnEvent(e);
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
		dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));

	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// 例如, 我们用 'R' 键来开始/停止录音
		if (e.GetKeyCode() == Key::R && e.GetRepeatCount() == 0) 
		{
			m_AgentCore->ToggleRecordingAndProcess();
			return true; // 事件已处理
		}
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
		}
		return false;
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
			Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera, camera.GetComponent<TransformComponent>().GetTransform());
		}
		else
		{
			Renderer2D::BeginScene(m_EditorCamera);
		}

		Renderer2D::EndScene();
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_EditorScenePath = std::filesystem::path();
	}
	void EditorLayer::OpenScene()
	{
		std::optional<std::string> filepath = FileDialogs::OpenFile("Razel Scene (*.razel)\0*.razel\0");
		if (filepath)
		{
			OpenScene(filepath->c_str());
		}
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
			OnSceneStop();

		if (path.extension().string() != ".razel")
		{
			RZ_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}
		Ref<Scene> newScene = CreateRef<Scene>();
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			m_EditorScene = newScene;
			m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

			m_ActiveScene = m_EditorScene;
			m_EditorScenePath = path;
		}

	}

	void EditorLayer::SaveScene()
	{
		if (!m_EditorScenePath.empty())
			SerializeScene(m_ActiveScene, m_EditorScenePath);
		else
			SaveSceneAs();
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = FileDialogs::SaveFile("Razel Scene (*.razel)\0*.razel\0");
		if (!filepath.empty())
		{
			SerializeScene(m_ActiveScene, filepath);
			m_EditorScenePath = filepath;
		}
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path& path)
	{
		SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}

	void EditorLayer::OnScenePlay()
	{
		m_SceneState = SceneState::Play;
		m_ActiveScene = Scene::Copy(m_EditorScene);
		m_ActiveScene->OnRuntimeStart();
	}

	void EditorLayer::OnSceneStop()
	{
		m_SceneState = SceneState::Edit;
		m_ActiveScene->OnRuntimeStop();
		m_ActiveScene = m_EditorScene;
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;
	}

	void EditorLayer::UpdateVoiceAssistantModel(float ts)
	{
		if (!m_VoiceAssistantEntity.IsValid())
			return;

		auto& transformComponent = m_VoiceAssistantEntity.GetComponent<TransformComponent>();
		AgentState currentState = m_AgentCore->GetCurrentState();

		switch (currentState)
		{
		case AgentState::Idle:
		{
			// 水平缓慢转动表示空闲状态
			m_IdleRotation += ts * 20.0f; // 每秒转动20度
			transformComponent.Rotation = glm::vec3(0.0f, m_IdleRotation, 0.0f);
			break;
		}
		case AgentState::Listening:
		{
			// 从当前状态变换到向右向下偏转角度
			// 使用插值使变换更平滑
			glm::vec3 currentRotation = transformComponent.Rotation;
			glm::vec3 targetRotation = m_TargetListeningRotation;

			// 插值计算
			glm::vec3 delta = targetRotation - currentRotation;
			float interpolationSpeed = 5.0f; // 插值速度

			if (glm::length(delta) > 0.1f) {
				transformComponent.Rotation += delta * ts * interpolationSpeed;
			}
			else {
				transformComponent.Rotation = targetRotation;
			}
			break;
		}
		case AgentState::Processing:
		{
			// 快速转动
			m_IdleRotation += ts * 100.0f; // 每秒转动100度（比空闲状态快5倍）
			transformComponent.Rotation = glm::vec3(0.0f, m_IdleRotation, 0.0f);
			break;
		}
		case AgentState::Speaking:
		{
			// 沿着x轴一上一下旋转
			m_SpeakingBobTime += ts * m_SpeakingBobSpeed;
			float bobAngle = sin(m_SpeakingBobTime) * m_SpeakingBobIntensity;
			transformComponent.Rotation = glm::vec3(bobAngle, 0.0f, 0.0f);
			break;
		}
		}
	}

}