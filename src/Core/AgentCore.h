#pragma once

//TODO:
class AgentCore
{
public:

	enum class AgentState
	{
		Idle,Listening,Processing,Speaking
	};

	void GetCurrentState();
	void OnUpdate();

private:
};

