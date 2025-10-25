#pragma once

#include "ToolRegistry.h"
#include "ITool.h"
#include <string>

namespace Razel {

    //=======System Tools=========
    // 获取当前时间日期
    class GetCurrentTimeTool : public ITool {
    public:

        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    // 获取系统信息工具 - CPU、内存、磁盘使用情况
    class GetSystemInfoTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    // 获取网络状态工具 - 检查网络连接
    class GetNetworkStatusTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    // 获取电池状态工具 - 电量、充电状态等
    class GetBatteryStatusTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    //=======File Tools=========
    //将文本内容写入文件的工具。
    class WriteFileTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    //读取文件内容的工具
    class ReadFileTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    //创建目录的工具
    class CreateDirectoryTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };

    //复制文件的工具
    class CopyFileTool : public ITool {
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

    //获取常用系统文件夹路径 (如桌面、文档)。 
    class ListDirectoryTool : public ITool {
    public:
        std::string Execute(const nlohmann::json& args) override;
        nlohmann::json GetDefinition() const override;
    };


    //=======App Tools=========
    //打开应用程序
	class GetAvailableApplicationsTool : public ITool {
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
    
    //打开应用程序(自执行)
	class SmartOpenApplicationTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};
    //关闭应用程序
	class CloseProcessTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};    
    //关闭应用程序
	class SmartCloseApplicationTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};
    //=======Shell Tools=========
}