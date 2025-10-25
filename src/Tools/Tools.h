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

	//=======Web Tools========= (新增)
    // 在浏览器中打开一个URL
	class OpenURLTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	// 在浏览器中执行网络搜索
	class WebSearchTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	// 获取网页的文本内容
	class FetchWebpageContentTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};
	//=======Window Management Tools========
    // 获取当前活动窗口的标题
	class GetActiveWindowTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	// 切换到指定窗口
	class SwitchWindowTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	// 设置窗口的状态 (最小化/最大化/还原)
	class SetWindowStateTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

	class MediaControlTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};
    //=======Shell Tools=========
	// 执行Shell命令并返回输出
	class ExecuteShellCommandTool : public ITool {
	public:
		std::string Execute(const nlohmann::json& args) override;
		nlohmann::json GetDefinition() const override;
	};

}