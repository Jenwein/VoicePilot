#pragma once

#include "ToolRegistry.h"
#include <string>

namespace Razel {
	// 获取当前时间日期
	class GetCurrentTimeTool : public ITool {
	public:

		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	//将文本内容写入文件的工具。
	class WriteFileTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	//获取常用系统文件夹路径 (如桌面、文档)。 
	class GetKnownFolderPathTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	//打开应用程序
	class OpenApplicationTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

}