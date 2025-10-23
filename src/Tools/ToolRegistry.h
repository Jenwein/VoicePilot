#pragma once
#include <string>
#include <map>
#include <memory>
#include <functional>

#include <nlohmann/json.hpp>
namespace Razel
{
	class ITool;
	// 单例工具的注册和管理类
    class ToolRegistry {
    public:
        static ToolRegistry& GetInstance() {
            static ToolRegistry instance;
            return instance;
        }

        ToolRegistry(const ToolRegistry&) = delete;
        ToolRegistry& operator=(const ToolRegistry&) = delete;

        template<typename T>
        void RegisterTool(const std::string& name) {
            static_assert(std::is_base_of<ITool, T>::value, "T must be a descendant of ITool");
            m_Tools[name] = std::make_unique<T>();
            std::cout << "[ToolRegistry] Registered tool: " << name << std::endl;
        }

        std::string ExecuteTool(const std::string& name, const nlohmann::json& args);

		bool HasTool(const std::string& name) const {
			return m_Tools.find(name) != m_Tools.end();
		}

        //获取所有已注册工具的定义
        nlohmann::json GetAllToolDefinitions() const;
    private:

        ToolRegistry() = default;
        std::map<std::string, std::unique_ptr<ITool>> m_Tools;  // 存储所有已注册的工具
    };
}

