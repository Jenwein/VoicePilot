#pragma once

#include <Razel.h>
#include <atomic>

#include "Razel/Renderer/EditorCamera.h"
#include "Core/AgentCore.h"

#include "Python/PythonManager.h"
namespace Razel {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void OnOverlayRender();

		void NewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& path);
		void SaveScene();
		void SaveSceneAs();

		void SerializeScene(Ref<Scene> scene, const std::filesystem::path& path);

		void OnScenePlay();
		void OnSceneStop();

		void OnDuplicateEntity();

		void UpdateVoiceAssistantModel(float ts);
		// UI Panels
		void UI_ToolBars();
	private:
		Razel::OrthographicCameraController m_CameraController;

		// Temp
		Ref<VertexArray> m_SquareVA;
		Ref<Shader> m_FlatColorShader;
		Ref<Framebuffer> m_Framebuffer;

		Ref<Texture2D> m_CheckerboardTexture;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene;
		std::filesystem::path m_EditorScenePath;
		Entity m_SquareEntity;
		Entity m_CameraEntity;
		Entity m_SecondCamera;
		Entity m_HoveredEntity;
		bool m_PrimaryCamera = true;

		EditorCamera m_EditorCamera;

		glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
		glm::vec2 m_ViewportBounds[2] = { {} ,{} };
		
		int m_GizmoType = -1;

		bool m_ShowPhysicsColliders = false;

		enum class SceneState
		{
			Edit = 0, Play = 1
		};
		SceneState m_SceneState = SceneState::Edit;
		
		// Panels
		//SceneHierarchyPanel m_SceneHierarchyPanel;
		//ContentBrowserPanel m_ContentBrowserPanel;

		bool m_ViewportFocused = false;
		bool m_ViewportHovered = false;
	
		// Editor resources
		Ref<Texture2D> m_IconPlay, m_IconStop;


		Scope<PythonCILRelease> m_GilGuard;
		Scope<AgentCore> m_AgentCore;
		Entity m_VoiceAssistantEntity;

		// Voice assistant model animation variables
		float m_IdleRotation = 0.0f;
		glm::vec3 m_ListeningRotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_TargetListeningRotation = { -20.0f, 30.0f, 0.0f }; // 向右向下偏转的角度
		glm::vec3 m_ProcessingRotationSpeed = { 0.0f, 150.0f, 0.0f }; // 快速转动速度
		glm::vec3 m_SpeakingRotation = { 0.0f, 0.0f, 0.0f };
		float m_SpeakingBobIntensity = 15.0f; // 上下摆动幅度
		float m_SpeakingBobSpeed = 5.0f; // 摆动速度
		float m_SpeakingBobTime = 0.0f;
	};
}