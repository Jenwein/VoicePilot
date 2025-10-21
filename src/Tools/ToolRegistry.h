#pragma once
#include <string>
#include <map>
#include <memory>
#include <functional>

#include <nlohmann/json.hpp>
namespace Razel
{
    // 抽象工具类
    class ITool {
    public:
        virtual ~ITool() = default;

		// 执行具体的工具方法, 接收参数并返回结果字符串
        virtual std::string Execute(const nlohmann::json& args) = 0;

		// 获取工具的定义，用于 LLM Function Call
        virtual nlohmann::json GetDefinition() const = 0;
    };

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

        //获取所有已注册工具的定义
        nlohmann::json GetAllToolDefinitions() const;
    private:

        ToolRegistry() = default;
        std::map<std::string, std::unique_ptr<ITool>> m_Tools;  // 存储所有已注册的工具
    };
}

