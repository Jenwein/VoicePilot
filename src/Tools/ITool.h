#pragma once
#include <string>
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
}