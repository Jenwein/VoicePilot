#include "../Tools/Tools.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h> 
#include <wininet.h>
#include <psapi.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <iphlpapi.h>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif

namespace Razel {
	std::string formatFileSize(std::uintmax_t size) {
		const char* units[] = { "B", "KB", "MB", "GB", "TB" };
		int unitIndex = 0;
		double displaySize = static_cast<double>(size);

		while (displaySize >= 1024.0 && unitIndex < 4) {
			displaySize /= 1024.0;
			unitIndex++;
		}

		std::stringstream ss;
		ss << std::fixed << std::setprecision(1) << displaySize << " " << units[unitIndex];
		return ss.str();
	}
	std::string getCurrentTimeString() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm;

		localtime_s(&now_tm, &now_c);

		std::stringstream ss;
		ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	std::string GetCurrentTimeTool::Execute(const nlohmann::json& args) {
		return "Success: Current time is " + getCurrentTimeString();
	}

	nlohmann::json GetCurrentTimeTool::GetDefinition() const {
		return {
			{"name", "get_current_time"},
			{"description", "Gets the current local date and time."},
			{"parameters", {
				{"type", "object"},
				{"properties", nlohmann::json::object()},
				{"required", nlohmann::json::array()}
			}}
		};
	}

	std::string WriteFileTool::Execute(const nlohmann::json& args) {
		if (!args.contains("path") || !args.contains("content")) {
			return "Error: Missing required arguments 'path' or 'content'.";
		}

		try {
			std::filesystem::path filePath(args["path"].get<std::string>());
			std::string content = args["content"].get<std::string>();

			if (filePath.has_parent_path()) {
				std::filesystem::create_directories(filePath.parent_path());
			}

			std::ofstream outFile(filePath);
			if (!outFile) {
				return "Error: Failed to open file for writing at " + filePath.string();
			}
			outFile << content;
			outFile.close();

			return "Success: Content successfully written to " + filePath.string();
		}
		catch (const std::exception& e) {
			return "Error: An exception occurred while writing to file. Details: " + std::string(e.what());
		}
	}

	nlohmann::json WriteFileTool::GetDefinition() const {
		return {
			{"name", "write_to_file"},
			{"description", "Writes the specified text content to a file. It creates the file if it does not exist, and overwrites it if it exists."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"content", {
						{"type", "string"},
						{"description", "The text content to write to the file."}
					}},
					{"path", {
						{"type", "string"},
						{"description", "The full path of the file to write to, e.g., 'C:/Users/User/Desktop/example.txt'."}
					}}
				}},
				{"required", {"content", "path"}}
			}}
		};
	}
	std::string GetKnownFolderPathTool::Execute(const nlohmann::json& args) {
		if (!args.contains("folder_name")) {
			return "Error: Missing required argument 'folder_name'.";
		}
		std::string folderName = args["folder_name"].get<std::string>();

		KNOWNFOLDERID folderId;
		if (folderName == "desktop") {
			folderId = FOLDERID_Desktop;
		}
		else if (folderName == "documents") {
			folderId = FOLDERID_Documents;
		}
		else if (folderName == "downloads") {
			folderId = FOLDERID_Downloads;
		}
		else {
			return "Error: Unsupported folder name. Use 'desktop', 'documents', or 'downloads'.";
		}

		PWSTR pszPath = NULL;
		HRESULT hr = SHGetKnownFolderPath(folderId, 0, NULL, &pszPath);

		if (SUCCEEDED(hr)) {
			std::wstring widePath(pszPath);
			CoTaskMemFree(pszPath);

			// Convert wstring to string
			std::filesystem::path p(widePath);
			return "Success: " + p.string();
		}
		else {
			return "Error: Failed to get known folder path.";
		}
	}

	nlohmann::json GetKnownFolderPathTool::GetDefinition() const {
		return {
			{"name", "get_known_folder_path"},
			{"description", "Gets the absolute path of a standard system folder."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"folder_name", {
						{"type", "string"},
						{"description", "The name of the folder to query. Supported values: 'desktop', 'documents', 'downloads','pictures', 'music', 'videos'."}
					}}
				}},
				{"required", {"folder_name"}}
			}}
		};
	}
	std::string ReadFileTool::Execute(const nlohmann::json& args) {
        if (!args.contains("path")) {
            return "Error: Missing required argument 'path'.";
        }

        try {
            std::filesystem::path filePath(args["path"].get<std::string>());
            
            if (!std::filesystem::exists(filePath)) {
                return "Error: File does not exist at " + filePath.string();
            }

            if (!std::filesystem::is_regular_file(filePath)) {
                return "Error: Path is not a regular file: " + filePath.string();
            }

            std::ifstream inFile(filePath);
            if (!inFile) {
                return "Error: Failed to open file for reading at " + filePath.string();
            }

            std::string content;
            std::string line;
            while (std::getline(inFile, line)) {
                content += line + "\n";
            }
            inFile.close();

            return "Success: File content read successfully.\nContent:\n" + content;
        }
        catch (const std::exception& e) {
            return "Error: An exception occurred while reading file. Details: " + std::string(e.what());
        }
    }

    nlohmann::json ReadFileTool::GetDefinition() const {
        return {
            {"name", "read_file"},
            {"description", "Reads the content from a specified file and returns it as text."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "The full path of the file to read from, e.g., 'C:/Users/User/Desktop/example.txt'."}
                    }}
                }},
                {"required", {"path"}}
            }}
        };
    }

    std::string CreateDirectoryTool::Execute(const nlohmann::json& args) {
        if (!args.contains("path")) {
            return "Error: Missing required argument 'path'.";
        }

        try {
            std::filesystem::path dirPath(args["path"].get<std::string>());
            
            if (std::filesystem::exists(dirPath)) {
                if (std::filesystem::is_directory(dirPath)) {
                    return "Success: Directory already exists at " + dirPath.string();
                } else {
                    return "Error: A file with the same name already exists at " + dirPath.string();
                }
            }

            bool created = std::filesystem::create_directories(dirPath);
            if (created) {
                return "Success: Directory created successfully at " + dirPath.string();
            } else {
                return "Error: Failed to create directory at " + dirPath.string();
            }
        }
        catch (const std::exception& e) {
            return "Error: An exception occurred while creating directory. Details: " + std::string(e.what());
        }
    }

    nlohmann::json CreateDirectoryTool::GetDefinition() const {
        return {
            {"name", "create_directory"},
            {"description", "Creates a new directory at the specified path. Creates parent directories if they don't exist."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "The full path of the directory to create, e.g., 'C:/Users/User/Desktop/NewFolder'."}
                    }}
                }},
                {"required", {"path"}}
            }}
        };
    }

    std::string CopyFileTool::Execute(const nlohmann::json& args) {
        if (!args.contains("source_path") || !args.contains("destination_path")) {
            return "Error: Missing required arguments 'source_path' or 'destination_path'.";
        }

        try {
            std::filesystem::path sourcePath(args["source_path"].get<std::string>());
            std::filesystem::path destPath(args["destination_path"].get<std::string>());
            
            if (!std::filesystem::exists(sourcePath)) {
                return "Error: Source file does not exist at " + sourcePath.string();
            }

            if (!std::filesystem::is_regular_file(sourcePath)) {
                return "Error: Source path is not a regular file: " + sourcePath.string();
            }

            // 如果目标路径的父目录不存在，创建它
            if (destPath.has_parent_path() && !std::filesystem::exists(destPath.parent_path())) {
                std::filesystem::create_directories(destPath.parent_path());
            }

            // 检查是否要覆盖现有文件
            bool overwrite = false;
            if (args.contains("overwrite")) {
                overwrite = args["overwrite"].get<bool>();
            }

            if (std::filesystem::exists(destPath) && !overwrite) {
                return "Error: Destination file already exists at " + destPath.string() + ". Use 'overwrite: true' to replace it.";
            }

            std::filesystem::copy_file(sourcePath, destPath, 
                overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none);

            return "Success: File copied successfully from " + sourcePath.string() + " to " + destPath.string();
        }
        catch (const std::exception& e) {
            return "Error: An exception occurred while copying file. Details: " + std::string(e.what());
        }
    }

    nlohmann::json CopyFileTool::GetDefinition() const {
        return {
            {"name", "copy_file"},
            {"description", "Copies a file from source path to destination path. Creates destination directories if they don't exist."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"source_path", {
                        {"type", "string"},
                        {"description", "The full path of the source file to copy, e.g., 'C:/Users/User/Desktop/source.txt'."}
                    }},
                    {"destination_path", {
                        {"type", "string"},
                        {"description", "The full path where the file should be copied to, e.g., 'C:/Users/User/Documents/copy.txt'."}
                    }},
                    {"overwrite", {
                        {"type", "boolean"},
                        {"description", "Whether to overwrite the destination file if it already exists. Default is false."}
                    }}
                }},
                {"required", {"source_path", "destination_path"}}
            }}
        };
    }

	std::string GetSystemInfoTool::Execute(const nlohmann::json& args) {
        try {
            std::stringstream result;
            result << "Success: System Information:\n";

#ifdef _WIN32
            // 获取内存信息
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            if (GlobalMemoryStatusEx(&memInfo)) {
                DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
                DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
                DWORDLONG totalVirtualMem = memInfo.ullTotalVirtual;
                DWORDLONG virtualMemUsed = memInfo.ullTotalVirtual - memInfo.ullAvailVirtual;

                result << "\nMemory Information:\n";
                result << "  Total Physical Memory: " << (totalPhysMem / (1024 * 1024)) << " MB\n";
                result << "  Used Physical Memory: " << (physMemUsed / (1024 * 1024)) << " MB\n";
                result << "  Memory Usage: " << memInfo.dwMemoryLoad << "%\n";
                result << "  Total Virtual Memory: " << (totalVirtualMem / (1024 * 1024)) << " MB\n";
                result << "  Used Virtual Memory: " << (virtualMemUsed / (1024 * 1024)) << " MB\n";
            }

            // 获取CPU信息
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            result << "\nCPU Information:\n";
            result << "  Number of Processors: " << sysInfo.dwNumberOfProcessors << "\n";
            result << "  Processor Architecture: ";
            switch (sysInfo.wProcessorArchitecture) {
                case PROCESSOR_ARCHITECTURE_AMD64:
                    result << "x64 (AMD or Intel)";
                    break;
                case PROCESSOR_ARCHITECTURE_ARM:
                    result << "ARM";
                    break;
                case PROCESSOR_ARCHITECTURE_IA64:
                    result << "Intel Itanium";
                    break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                    result << "x86";
                    break;
                default:
                    result << "Unknown";
                    break;
            }
            result << "\n";

            // 获取磁盘信息
            result << "\nDisk Information:\n";
            DWORD drives = GetLogicalDrives();
            for (char drive = 'A'; drive <= 'Z'; drive++) {
                if (drives & (1 << (drive - 'A'))) {
                    std::string drivePath = std::string(1, drive) + ":\\";
                    UINT driveType = GetDriveTypeA(drivePath.c_str());
                    
                    if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                        ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                        if (GetDiskFreeSpaceExA(drivePath.c_str(), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                            ULONGLONG totalGB = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024);
                            ULONGLONG freeGB = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024);
                            ULONGLONG usedGB = totalGB - freeGB;
                            double usagePercent = totalGB > 0 ? (double)usedGB / totalGB * 100.0 : 0.0;

                            result << "  Drive " << drive << ": ";
                            result << "Total: " << totalGB << " GB, ";
                            result << "Used: " << usedGB << " GB, ";
                            result << "Free: " << freeGB << " GB ";
                            result << "(" << std::fixed << std::setprecision(1) << usagePercent << "% used)\n";
                        }
                    }
                }
            }
#endif
            return result.str();
        }
        catch (const std::exception& e) {
            return "Error: Failed to get system information. Details: " + std::string(e.what());
        }
    }

    nlohmann::json GetSystemInfoTool::GetDefinition() const {
        return {
            {"name", "get_system_info"},
            {"description", "Gets comprehensive system information including CPU, memory, and disk usage statistics."},
            {"parameters", {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}
            }}
        };
    }

    std::string GetNetworkStatusTool::Execute(const nlohmann::json& args) {
        try {
            std::stringstream result;
            result << "Success: Network Status:\n";

#ifdef _WIN32
            // 检查基本网络连接状态
            DWORD flags;
            BOOL isConnected = InternetGetConnectedState(&flags, 0);
            
            result << "\nConnection Status: " << (isConnected ? "Connected" : "Disconnected") << "\n";
            
            if (isConnected) {
                result << "Connection Type: ";
                if (flags & INTERNET_CONNECTION_MODEM) result << "Modem ";
                if (flags & INTERNET_CONNECTION_LAN) result << "LAN ";
                if (flags & INTERNET_CONNECTION_PROXY) result << "Proxy ";
                if (flags & INTERNET_CONNECTION_MODEM_BUSY) result << "Modem_Busy ";
                result << "\n";

                // 测试具体的网络连接
                result << "\nConnectivity Test:\n";
                
                // 测试连接到知名网站
                HINTERNET hInternet = InternetOpenA("NetworkTest", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                if (hInternet) {
                    HINTERNET hConnect = InternetOpenUrlA(hInternet, "http://www.google.com", NULL, 0, INTERNET_FLAG_RELOAD, 0);
                    if (hConnect) {
                        result << "  Internet Access: Available\n";
                        InternetCloseHandle(hConnect);
                    } else {
                        result << "  Internet Access: Limited or Unavailable\n";
                    }
                    InternetCloseHandle(hInternet);
                } else {
                    result << "  Internet Access: Cannot test\n";
                }
            }

            // 获取网络适配器信息
            result << "\nNetwork Adapters:\n";
            IP_ADAPTER_INFO* pAdapterInfo = nullptr;
            ULONG bufferSize = 0;
            
            // 获取需要的缓冲区大小
            GetAdaptersInfo(pAdapterInfo, &bufferSize);
            if (bufferSize > 0) {
                pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
                if (pAdapterInfo) {
                    if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == NO_ERROR) {
                        IP_ADAPTER_INFO* pAdapter = pAdapterInfo;
                        int adapterCount = 0;
                        while (pAdapter && adapterCount < 5) { // 限制显示前5个适配器
                            result << "  Adapter " << (adapterCount + 1) << ": " << pAdapter->Description << "\n";
                            result << "    IP Address: " << pAdapter->IpAddressList.IpAddress.String << "\n";
                            result << "    Subnet Mask: " << pAdapter->IpAddressList.IpMask.String << "\n";
                            result << "    Gateway: " << pAdapter->GatewayList.IpAddress.String << "\n";
                            
                            pAdapter = pAdapter->Next;
                            adapterCount++;
                        }
                    }
                    free(pAdapterInfo);
                }
            }
#endif
            return result.str();
        }
        catch (const std::exception& e) {
            return "Error: Failed to get network status. Details: " + std::string(e.what());
        }
    }

    nlohmann::json GetNetworkStatusTool::GetDefinition() const {
        return {
            {"name", "get_network_status"},
            {"description", "Gets current network connection status, connectivity information, and network adapter details."},
            {"parameters", {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}
            }}
        };
    }

    std::string GetBatteryStatusTool::Execute(const nlohmann::json& args) {
        try {
            std::stringstream result;

#ifdef _WIN32
            SYSTEM_POWER_STATUS powerStatus;
            if (GetSystemPowerStatus(&powerStatus)) {
                result << "Success: Battery Status:\n";
                
                // AC电源状态
                result << "\nAC Power: ";
                switch (powerStatus.ACLineStatus) {
                    case 0:
                        result << "Offline (Running on battery)";
                        break;
                    case 1:
                        result << "Online (Plugged in)";
                        break;
                    case 255:
                        result << "Unknown";
                        break;
                    default:
                        result << "Unknown";
                        break;
                }
                result << "\n";

                // 电池状态
                result << "Battery Status: ";
                switch (powerStatus.BatteryFlag) {
                    case 1:
                        result << "High (> 66%)";
                        break;
                    case 2:
                        result << "Low (< 33%)";
                        break;
                    case 4:
                        result << "Critical (< 5%)";
                        break;
                    case 8:
                        result << "Charging";
                        break;
                    case 128:
                        result << "No battery";
                        break;
                    case 255:
                        result << "Unknown";
                        break;
                    default:
                        if (powerStatus.BatteryFlag & 8) {
                            result << "Charging";
                        } else {
                            result << "Not charging";
                        }
                        break;
                }
                result << "\n";

                // 电池电量百分比
                if (powerStatus.BatteryLifePercent != 255) {
                    result << "Battery Level: " << (int)powerStatus.BatteryLifePercent << "%\n";
                } else {
                    result << "Battery Level: Unknown\n";
                }

                // 剩余时间
                if (powerStatus.BatteryLifeTime != 0xFFFFFFFF) {
                    int hours = powerStatus.BatteryLifeTime / 3600;
                    int minutes = (powerStatus.BatteryLifeTime % 3600) / 60;
                    result << "Remaining Time: " << hours << " hours " << minutes << " minutes\n";
                } else {
                    result << "Remaining Time: Unknown\n";
                }

                // 节能模式
                result << "Power Saver Mode: ";
                switch (powerStatus.SystemStatusFlag) {
                    case 0:
                        result << "Off";
                        break;
                    case 1:
                        result << "On";
                        break;
                    default:
                        result << "Unknown";
                        break;
                }
                result << "\n";

            } else {
                return "Error: Failed to retrieve battery status information.";
            }
#else
            return "Error: Battery status is only supported on Windows systems.";
#endif
            return result.str();
        }
        catch (const std::exception& e) {
            return "Error: Failed to get battery status. Details: " + std::string(e.what());
        }
    }

    nlohmann::json GetBatteryStatusTool::GetDefinition() const {
        return {
            {"name", "get_battery_status"},
            {"description", "Gets current battery status including charge level, charging state, remaining time, and power source information."},
            {"parameters", {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}
            }}
        };
    }
    //===========App Tools==========
	// 添加应用名称映射表
	std::map<std::string, std::vector<std::string>> getAppNameMappings() {
		static std::map<std::string, std::vector<std::string>> mappings = {
			// 社交通讯
			{"微信", {"WeChat.exe", "WeChatApp.exe"}},
			{"QQ", {"QQ.exe", "QQScLauncher.exe"}},
			{"钉钉", {"DingTalk.exe"}},
			{"腾讯会议", {"TencentMeeting.exe", "wemeet.exe"}},

			// 浏览器
			{"浏览器", {"chrome.exe", "firefox.exe", "msedge.exe", "iexplore.exe"}},
			{"Chrome", {"chrome.exe"}},
			{"火狐", {"firefox.exe"}},
			{"Edge", {"msedge.exe"}},
			{"谷歌浏览器", {"chrome.exe"}},

			// 办公软件
			{"Word", {"WINWORD.exe"}},
			{"Excel", {"EXCEL.exe"}},
			{"PowerPoint", {"POWERPNT.exe"}},
			{"记事本", {"notepad.exe"}},
			{"写字板", {"wordpad.exe"}},

			// 开发工具
			{"Visual Studio Code", {"Code.exe"}},
			{"VS Code", {"Code.exe"}},
			{"Visual Studio", {"devenv.exe"}},

			// 媒体播放
			{"网易云音乐", {"cloudmusic.exe"}},
			{"QQ音乐", {"QQMusic.exe"}},
			{"VLC", {"vlc.exe"}},

			// 其他常用
			{"计算器", {"calc.exe", "Calculator.exe"}},
			{"任务管理器", {"Taskmgr.exe"}},
			{"资源管理器", {"explorer.exe"}},
			{"画图", {"mspaint.exe"}}
		};
		return mappings;
	}

	std::vector<std::string> findProcessNames(const std::string& appName) {
		auto mappings = getAppNameMappings();

		// 首先检查精确匹配
		auto it = mappings.find(appName);
		if (it != mappings.end()) {
			return it->second;
		}

		// 然后检查模糊匹配（包含关系）
		std::vector<std::string> results;
		std::string lowerAppName = appName;
		std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::tolower);

		for (const auto& pair : mappings) {
			std::string lowerKey = pair.first;
			std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

			if (lowerKey.find(lowerAppName) != std::string::npos ||
				lowerAppName.find(lowerKey) != std::string::npos) {
				results.insert(results.end(), pair.second.begin(), pair.second.end());
			}
		}

		// 如果没有找到映射，直接尝试作为进程名使用
		if (results.empty()) {
			results.push_back(appName);
			// 如果不包含.exe，自动添加
			if (appName.find(".exe") == std::string::npos) {
				results.push_back(appName + ".exe");
			}
		}

		return results;
	}
	std::string GetAvailableApplicationsTool::Execute(const nlohmann::json& args) {
		try {
			// 获取配置文件路径
			std::filesystem::path configPath = std::filesystem::current_path() / "Resources" / "appinfos" / "app.json";

			// 检查配置文件是否存在
			if (!std::filesystem::exists(configPath)) {
				return "Error: Application configuration file not found at " + configPath.string();
			}

			// 读取配置文件
			std::ifstream configFile(configPath);
			if (!configFile.is_open()) {
				return "Error: Failed to open application configuration file.";
			}

			nlohmann::json appConfig;
			try {
				configFile >> appConfig;
			}
			catch (const nlohmann::json::parse_error& e) {
				return "Error: Failed to parse application configuration file. Invalid JSON format.";
			}
			configFile.close();

			// 检查配置文件结构
			if (!appConfig.contains("applications")) {
				return "Error: Invalid configuration file format. Missing 'applications' section.";
			}

			std::stringstream result;
			result << "Success: Available Applications:\n\n";

			auto applications = appConfig["applications"];
			int appCount = 0;

			for (auto it = applications.begin(); it != applications.end(); ++it) {
				appCount++;
				std::string appId = it.key();
				auto appInfo = it.value();

				result << "Application " << appCount << ":\n";
				result << "  ID: " << appId << "\n";

				// 显示别名
				if (appInfo.contains("aliases") && appInfo["aliases"].is_array()) {
					result << "  Aliases: ";
					auto aliases = appInfo["aliases"];
					for (size_t i = 0; i < aliases.size(); ++i) {
						result << "\"" << aliases[i].get<std::string>() << "\"";
						if (i < aliases.size() - 1) {
							result << ", ";
						}
					}
					result << "\n";
				}

				// 显示路径
				if (appInfo.contains("path")) {
					std::string path = appInfo["path"].get<std::string>();
					result << "  Path: " << path << "\n";

					// 检查文件是否存在（如果是绝对路径）
					if (std::filesystem::path(path).is_absolute()) {
						bool exists = std::filesystem::exists(path);
						result << "  Status: " << (exists ? "Available" : "Not Found") << "\n";
					}
					else {
						result << "  Status: System Application\n";
					}
				}

				// 显示进程名
				if (appInfo.contains("processName")) {
					result << "  Process Name: " << appInfo["processName"].get<std::string>() << "\n";
				}

				result << "\n";
			}

			result << "Total Applications: " << appCount << "\n";
			result << "\nUsage: Use the aliases or application ID when requesting to open/close applications.";

			return result.str();
		}
		catch (const std::exception& e) {
			return "Error: An exception occurred while reading application configuration. Details: " + std::string(e.what());
		}
	}

	nlohmann::json GetAvailableApplicationsTool::GetDefinition() const {
		return {
			{"name", "get_available_applications"},
			{"description", "Gets the list of available applications that can be controlled, including their aliases, paths, and current status."},
			{"parameters", {
				{"type", "object"},
				{"properties", nlohmann::json::object()},
				{"required", nlohmann::json::array()}
			}}
		};
	}

	std::string OpenApplicationTool::Execute(const nlohmann::json& args)
	{
		if (!args.contains("path_or_command")) {
			return "Error: Missing required argument 'path_or_command'.";
		}
		std::string command = args["path_or_command"].get<std::string>();

		try {
			// 直接使用 ShellExecuteA 尝试执行路径或命令
			HINSTANCE result = ShellExecuteA(NULL, "open", command.c_str(), NULL, NULL, SW_SHOWNORMAL);

			if ((intptr_t)result > 32) {
				return "Success: Executed command '" + command + "'.";
			}
			else {
				return "Error: Failed to execute command '" + command + "'. Error code: " + std::to_string((intptr_t)result);
			}
		}
		catch (const std::exception& e) {
			return "Error: An exception occurred while executing the command. Details: " + std::string(e.what());
		}
	}

	nlohmann::json OpenApplicationTool::GetDefinition() const
	{
		return {
			{"name", "open_application"},
			{"description", "Opens an application or file using its direct executable path or a system command (e.g., 'C:\\Windows\\notepad.exe', 'notepad'). This is a basic executor."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"path_or_command", {
						{"type", "string"},
						{"description", "The absolute path to the executable, a file path, or a command known to the system shell."}
					}}
				}},
				{"required", {"path_or_command"}}
			}}
		};
	}


	// 2. 新增 SmartOpenApplicationTool (作为智能工具)
	std::string SmartOpenApplicationTool::Execute(const nlohmann::json& args)
	{
		if (!args.contains("app_name")) {
			return "Error: Missing required argument 'app_name'.";
		}
		std::string appName = args["app_name"].get<std::string>();

		try {
			// 1. 读取配置文件
			std::filesystem::path configPath = std::filesystem::current_path() / "Resources" / "appinfos" / "app.json";
			if (!std::filesystem::exists(configPath)) {
				// 配置文件不存在时，直接尝试将 app_name 作为命令执行
				HINSTANCE result = ShellExecuteA(NULL, "open", appName.c_str(), NULL, NULL, SW_SHOWNORMAL);
				if ((intptr_t)result > 32) {
					return "Success: Application '" + appName + "' started (configuration file not found, attempted direct launch).";
				}
				else {
					return "Error: Application configuration file not found and failed to launch '" + appName + "' directly.";
				}
			}

			std::ifstream configFile(configPath);
			nlohmann::json appConfig;
			configFile >> appConfig;
			configFile.close();

			// 2. 在配置文件中查找应用路径
			std::string appPath;
			bool found = false;
			if (appConfig.contains("applications")) {
				auto apps = appConfig["applications"];
				for (auto it = apps.begin(); it != apps.end(); ++it) {
					std::string currentId = it.key();
					auto appInfo = it.value();

					// 检查ID匹配
					if (currentId == appName) {
						found = true;
					}
					// 检查别名匹配
					else if (appInfo.contains("aliases")) {
						for (const auto& alias : appInfo["aliases"]) {
							if (alias.get<std::string>() == appName) {
								found = true;
								break;
							}
						}
					}

					if (found) {
						if (appInfo.contains("path")) {
							appPath = appInfo["path"].get<std::string>();
						}
						break;
					}
				}
			}

			if (appPath.empty()) {
				// 3. 如果在配置中找不到，直接将 appName 作为可执行文件或系统命令尝试
				appPath = appName;
			}

			// 4. 执行打开操作
			HINSTANCE result = ShellExecuteA(NULL, "open", appPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

			if ((intptr_t)result > 32) {
				return "Success: Application '" + appName + "' started.";
			}
			else {
				// 错误码可以帮助调试
				return "Error: Failed to open application '" + appName + "' with path '" + appPath + "'. Error code: " + std::to_string((intptr_t)result);
			}
		}
		catch (const std::exception& e) {
			return "Error: An exception occurred while trying to open the application. Details: " + std::string(e.what());
		}
	}

	nlohmann::json SmartOpenApplicationTool::GetDefinition() const
	{
		return {
			{"name", "smart_open_application"},
			{"description", "Opens an application by its friendly name or alias. It intelligently finds the application's path from a configuration file."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"app_name", {
						{"type", "string"},
						{"description", "The friendly name or alias of the application to open. Use 'get_available_applications' to see the configured list."}
					}}
				}},
				{"required", {"app_name"}}
			}}
		};
	}

	std::string ListDirectoryTool::Execute(const nlohmann::json& args)
	{
		if (!args.contains("path")) {
			return "Error: Missing required argument 'path'.";
		}

		try {
			std::filesystem::path dirPath(args["path"].get<std::string>());

			if (!std::filesystem::exists(dirPath)) {
				return "Error: Directory does not exist at " + dirPath.string();
			}

			if (!std::filesystem::is_directory(dirPath)) {
				return "Error: Path is not a directory: " + dirPath.string();
			}

			std::stringstream result;
			result << "Success: Directory contents of " << dirPath.string() << ":\n\n";

			// 获取可选参数
			bool showHidden = false;
			bool showDetails = false;
			if (args.contains("show_hidden")) {
				showHidden = args["show_hidden"].get<bool>();
			}
			if (args.contains("show_details")) {
				showDetails = args["show_details"].get<bool>();
			}

			std::vector<std::filesystem::directory_entry> entries;

			// 收集目录项
			for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
				// 检查是否显示隐藏文件
				if (!showHidden) {
					std::string filename = entry.path().filename().string();
					if (!filename.empty() && filename[0] == '.') {
						continue; // 跳过隐藏文件
					}
				}
				entries.push_back(entry);
			}

			// 按名称排序，目录在前
			std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
				bool aIsDir = a.is_directory();
				bool bIsDir = b.is_directory();
				if (aIsDir != bIsDir) {
					return aIsDir > bIsDir; // 目录在前
				}
				return a.path().filename().string() < b.path().filename().string();
				});

			if (entries.empty()) {
				result << "Directory is empty.\n";
			}
			else {
				int fileCount = 0;
				int dirCount = 0;

				for (const auto& entry : entries) {
					std::string name = entry.path().filename().string();

					if (entry.is_directory()) {
						dirCount++;
						result << "[DIR]  ";
					}
					else {
						fileCount++;
						result << "[FILE] ";
					}

					result << name;

					if (showDetails) {
						try {
							if (entry.is_regular_file()) {
								auto size = std::filesystem::file_size(entry);
								result << " (" << formatFileSize(size) << ")";
							}

							auto time = std::filesystem::last_write_time(entry);
							auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
								time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
							);
							auto time_t = std::chrono::system_clock::to_time_t(sctp);
							std::tm tm;
							localtime_s(&tm, &time_t);

							result << " - Modified: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
						}
						catch (...) {
							// 忽略获取详细信息时的错误
						}
					}

					result << "\n";
				}

				result << "\nSummary: " << dirCount << " directories, " << fileCount << " files\n";
			}

			return result.str();
		}
		catch (const std::exception& e) {
			return "Error: An exception occurred while listing directory. Details: " + std::string(e.what());
		}
	}

	nlohmann::json ListDirectoryTool::GetDefinition() const
	{
		return {
		{"name", "list_directory"},
		{"description", "Lists the contents of a specified directory, showing files and subdirectories with optional details."},
		{"parameters", {
			{"type", "object"},
			{"properties", {
				{"path", {
					{"type", "string"},
					{"description", "The full path of the directory to list, e.g., 'C:/Users/User/Desktop'."}
				}},
				{"show_hidden", {
					{"type", "boolean"},
					{"description", "Whether to show hidden files and directories. Default is false."}
				}},
				{"show_details", {
					{"type", "boolean"},
					{"description", "Whether to show file sizes and modification dates. Default is false."}
				}}
			}},
			{"required", {"path"}}
		}}
		};
	}

	std::string CloseProcessTool::Execute(const nlohmann::json& args)
	{
		if (!args.contains("process_name")) {
			return "Error: Missing required argument 'process_name'.";
		}
		std::string processNameStr = args["process_name"].get<std::string>();
		int terminatedCount = 0;

		// 将要比较的进程名转换为宽字符串
		std::wstring processName(processNameStr.begin(), processNameStr.end());

		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE) {
			return "Error: Failed to create process snapshot.";
		}

		PROCESSENTRY32W pe32; // 明确使用宽字符版本 PROCESSENTRY32W
		pe32.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnap, &pe32)) { // 明确使用宽字符版本 Process32FirstW
			do {
				// 使用 _wcsicmp 进行不区分大小写的宽字符串比较
				if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
					if (hProcess) {
						if (TerminateProcess(hProcess, 1)) {
							terminatedCount++;
						}
						CloseHandle(hProcess);
					}
				}
			} while (Process32NextW(hSnap, &pe32)); // 明确使用宽字符版本 Process32NextW
		}

		CloseHandle(hSnap);

		if (terminatedCount > 0) {
			return "Success: Closed " + std::to_string(terminatedCount) + " process(es) named '" + processNameStr + "'.";
		}
		else {
			return "Error: Process '" + processNameStr + "' not found running.";
		}
	}

	nlohmann::json CloseProcessTool::GetDefinition() const
	{
		return {
			{"name", "close_process"},
			{"description", "Terminates a process based on its exact executable name."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"process_name", {
						{"type", "string"},
						{"description", "The exact executable name of the process to terminate, e.g., 'notepad.exe', 'chrome.exe'."}
					}}
				}},
				{"required", {"process_name"}}
			}}
		};
	}

	std::string SmartCloseApplicationTool::Execute(const nlohmann::json& args)
	{
		if (!args.contains("app_name")) {
			return "Error: Missing required argument 'app_name'.";
		}
		std::string appName = args["app_name"].get<std::string>();

		std::vector<std::string> processNames = findProcessNames(appName);
		int terminatedCount = 0;

		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE) {
			return "Error: Failed to create process snapshot.";
		}

		PROCESSENTRY32W pe32; // 明确使用宽字符版本
		pe32.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnap, &pe32)) { // 明确使用宽字符版本
			do {
				for (const auto& pNameStr : processNames) {
					// 将 std::string 转换为 std::wstring
					std::wstring pName(pNameStr.begin(), pNameStr.end());

					// 使用 _wcsicmp 进行宽字符串比较
					if (_wcsicmp(pe32.szExeFile, pName.c_str()) == 0) {
						HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
						if (hProcess) {
							if (TerminateProcess(hProcess, 1)) {
								terminatedCount++;
							}
							CloseHandle(hProcess);
						}
					}
				}
			} while (Process32NextW(hSnap, &pe32)); // 明确使用宽字符版本
		}

		CloseHandle(hSnap);

		if (terminatedCount > 0) {
			return "Success: Closed " + std::to_string(terminatedCount) + " process(es) for application '" + appName + "'.";
		}
		else {
			return "Error: Application '" + appName + "' not found running.";
		}
	}

	nlohmann::json SmartCloseApplicationTool::GetDefinition() const
	{
		return {
			{"name", "smart_close_application"},
			{"description", "Closes a running application by its friendly name or alias by intelligently finding the correct process."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"app_name", {
						{"type", "string"},
						{"description", "The friendly name or alias of the application to close, e.g., 'notepad', 'Chrome'."}
					}}
				}},
				{"required", {"app_name"}}
			}}
		};
	}

}