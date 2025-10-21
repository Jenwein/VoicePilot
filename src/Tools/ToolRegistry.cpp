#include "ToolRegistry.h"
#include <iostream>

namespace Razel {

	std::string ToolRegistry::ExecuteTool(const std::string& name, const nlohmann::json& args) {
		auto it = m_Tools.find(name);
		if (it != m_Tools.end()) {
			std::cout << "[ToolRegistry] Executing tool '" << name << "' with args: " << args.dump() << std::endl;
			try {
				return it->second->Execute(args);
			}
			catch (const std::exception& e) {
				std::cerr << "[ToolRegistry] Error executing tool '" << name << "': " << e.what() << std::endl;
				return "Error: " + std::string(e.what());
			}
		}
		else {
			std::cerr << "[ToolRegistry] Error: Tool '" << name << "' not found." << std::endl;
			return "Error: Tool not found.";
		}
	}

	nlohmann::json ToolRegistry::GetAllToolDefinitions() const
	{
		nlohmann::json definitions;
		nlohmann::json function_declarations = nlohmann::json::array();

		for (const auto& pair : m_Tools)
		{
			function_declarations.push_back(pair.second->GetDefinition());
		}

		definitions["function_declarations"] = function_declarations;
		return definitions;
	}

} // namespace Razel