#include <Razel.h>
#include <Razel/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Razel {

	class VoicePilot : public Application
	{
	public:
		VoicePilot()
			: Application("VoicePilot")
		{
			PushLayer(new EditorLayer());
		}

		~VoicePilot()
		{
		}
	};

	Application* CreateApplication()
	{
		return new VoicePilot();
	}

}