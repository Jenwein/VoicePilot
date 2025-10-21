#pragma once

namespace Razel
{
	enum class AgentState
	{
		Idle, Listening, Processing, Speaking
	};
	//TODO:
	class AgentCore
	{
	public:

		AgentState GetCurrentState() const { return m_CurrentState; }
		void OnUpdate();

	private:

		AgentState m_CurrentState = AgentState::Idle;
	};
}
