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
	//===========Web Tools==========

	std::string OpenURLTool::Execute(const nlohmann::json& args) {
		if (!args.contains("url")) {
			return "Error: Missing required argument 'url'.";
		}
		std::string url = args["url"].get<std::string>();

		// 使用 ShellExecuteA 在默认浏览器中打开URL
		HINSTANCE result = ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

		if ((intptr_t)result > 32) {
			return "Success: URL opened in default browser.";
		}
		else {
			return "Error: Failed to open URL. It might be malformed or no default browser is set.";
		}
	}

	nlohmann::json OpenURLTool::GetDefinition() const {
		return {
			{"name", "open_url"},
			{"description", "Opens a given URL in the user's default web browser."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"url", {
						{"type", "string"},
						{"description", "The full URL to open, e.g., 'https://www.google.com'."}
					}}
				}},
				{"required", {"url"}}
			}}
		};
	}

	std::string WebSearchTool::Execute(const nlohmann::json& args) {
		if (!args.contains("query")) {
			return "Error: Missing required argument 'query'.";
		}
		std::string query = args["query"].get<std::string>();

		// 对查询字符串进行简单的URL编码 (替换空格)
		std::string encoded_query = "";
		for (char c : query) {
			if (c == ' ') {
				encoded_query += '+';
			}
			else {
				encoded_query += c;
			}
		}

		// 构建Google搜索URL
		std::string search_url = "https://www.google.com/search?q=" + encoded_query;

		// 使用 ShellExecuteA 打开搜索结果页
		HINSTANCE result = ShellExecuteA(NULL, "open", search_url.c_str(), NULL, NULL, SW_SHOWNORMAL);

		if ((intptr_t)result > 32) {
			return "Success: Search results for '" + query + "' opened in browser.";
		}
		else {
			return "Error: Failed to perform web search.";
		}
	}

	nlohmann::json WebSearchTool::GetDefinition() const {
		return {
			{"name", "web_search"},
			{"description", "Performs a web search using Google and opens the results in the default browser."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"query", {
						{"type", "string"},
						{"description", "The search term or question to look up."}
					}}
				}},
				{"required", {"query"}}
			}}
		};
	}

	std::string FetchWebpageContentTool::Execute(const nlohmann::json& args) {
		if (!args.contains("url")) {
			return "Error: Missing required argument 'url'.";
		}
		std::string url = args["url"].get<std::string>();

		HINTERNET hInternet = InternetOpenA("RazelWebTool", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		if (!hInternet) {
			return "Error: Failed to initialize WinINet (InternetOpenA).";
		}

		HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
		if (!hConnect) {
			InternetCloseHandle(hInternet);
			return "Error: Failed to open URL. It might be invalid or the server is unreachable.";
		}

		std::string content = "";
		char buffer[4096];
		DWORD bytesRead = 0;

		while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
			buffer[bytesRead] = '\0'; // Null-terminate the buffer
			content += buffer;
		}

		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);

		if (content.empty()) {
			return "Error: Failed to read any content from the URL. The page might be empty or protected.";
		}

		return "Success: Fetched webpage content.\n\n" + content;
	}

	nlohmann::json FetchWebpageContentTool::GetDefinition() const {
		return {
			{"name", "fetch_webpage_content"},
			{"description", "Downloads and returns the raw HTML content of a given URL. Useful for analysis and summarization."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"url", {
						{"type", "string"},
						{"description", "The full URL of the webpage to fetch."}
					}}
				}},
				{"required", {"url"}}
			}}
		};
	}

	//===========Window Management Tools==========

// 辅助函数和数据结构，用于通过标题查找窗口
	struct EnumData {
		std::wstring title_substring;
		HWND hwnd = NULL;
	};

	BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
		EnumData* data = reinterpret_cast<EnumData*>(lParam);
		const int buffer_size = 256;
		wchar_t window_title[buffer_size];

		if (IsWindowVisible(hwnd) && GetWindowTextW(hwnd, window_title, buffer_size) > 0) {
			std::wstring current_title(window_title);
			// 不区分大小写的子字符串查找
			std::transform(current_title.begin(), current_title.end(), current_title.begin(), ::towlower);
			if (current_title.find(data->title_substring) != std::wstring::npos) {
				data->hwnd = hwnd;
				return FALSE; // 找到窗口，停止枚举
			}
		}
		return TRUE; // 继续枚举
	}

	HWND FindWindowByTitleSubstring(const std::string& title_substring) {
		EnumData data;
		std::wstring w_title_substring(title_substring.begin(), title_substring.end());
		std::transform(w_title_substring.begin(), w_title_substring.end(), w_title_substring.begin(), ::towlower);
		data.title_substring = w_title_substring;

		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
		return data.hwnd;
	}


	std::string GetActiveWindowTool::Execute(const nlohmann::json& args) {
		HWND hwnd = GetForegroundWindow();
		if (hwnd == NULL) {
			return "Success: No active window found.";
		}

		const int buffer_size = 256;
		wchar_t window_title_w[buffer_size];
		GetWindowTextW(hwnd, window_title_w, buffer_size);

		std::wstring w_title(window_title_w);
		if (w_title.empty()) {
			return "Success: Active window has no title.";
		}

		// 将宽字符串转换为UTF-8编码的string
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &w_title[0], (int)w_title.size(), NULL, 0, NULL, NULL);
		std::string title(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &w_title[0], (int)w_title.size(), &title[0], size_needed, NULL, NULL);

		return "Success: Active window title is \"" + title + "\"";
	}

	nlohmann::json GetActiveWindowTool::GetDefinition() const {
		return {
			{"name", "get_active_window"},
			{"description", "Gets the title of the currently active (foreground) window."},
			{"parameters", {
				{"type", "object"},
				{"properties", nlohmann::json::object()},
				{"required", nlohmann::json::array()}
			}}
		};
	}


	std::string SwitchWindowTool::Execute(const nlohmann::json& args) {
		if (!args.contains("window_title")) {
			return "Error: Missing required argument 'window_title'.";
		}
		std::string title_substring = args["window_title"].get<std::string>();

		HWND hwnd = FindWindowByTitleSubstring(title_substring);
		if (hwnd == NULL) {
			return "Error: Window with title containing '" + title_substring + "' not found.";
		}

		// 如果窗口最小化，先还原
		if (IsIconic(hwnd)) {
			ShowWindow(hwnd, SW_RESTORE);
		}

		// 切换到窗口
		if (SetForegroundWindow(hwnd)) {
			return "Success: Switched to window with title containing '" + title_substring + "'.";
		}
		else {
			return "Error: Failed to switch to the specified window.";
		}
	}

	nlohmann::json SwitchWindowTool::GetDefinition() const {
		return {
			{"name", "switch_window"},
			{"description", "Brings a window to the foreground, making it active. It finds the window by a substring of its title."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"window_title", {
						{"type", "string"},
						{"description", "A part of the title of the window to switch to, e.g., 'Notepad', 'Visual Studio'."}
					}}
				}},
				{"required", {"window_title"}}
			}}
		};
	}


	std::string SetWindowStateTool::Execute(const nlohmann::json& args) {
		if (!args.contains("window_title") || !args.contains("state")) {
			return "Error: Missing required arguments 'window_title' or 'state'.";
		}
		std::string title_substring = args["window_title"].get<std::string>();
		std::string state = args["state"].get<std::string>();
		std::transform(state.begin(), state.end(), state.begin(), ::tolower); // state不区分大小写

		HWND hwnd = FindWindowByTitleSubstring(title_substring);
		if (hwnd == NULL) {
			return "Error: Window with title containing '" + title_substring + "' not found.";
		}

		UINT cmd;
		if (state == "minimize") {
			cmd = SW_MINIMIZE;
		}
		else if (state == "maximize") {
			cmd = SW_MAXIMIZE;
		}
		else if (state == "restore") {
			cmd = SW_RESTORE;
		}
		else {
			return "Error: Invalid state specified. Use 'minimize', 'maximize', or 'restore'.";
		}

		if (ShowWindow(hwnd, cmd)) {
			return "Success: Window state changed to '" + state + "'.";
		}
		else {
			return "Error: Failed to change the window state.";
		}
	}

	nlohmann::json SetWindowStateTool::GetDefinition() const {
		return {
			{"name", "set_window_state"},
			{"description", "Minimizes, maximizes, or restores a window identified by a substring of its title."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"window_title", {
						{"type", "string"},
						{"description", "A part of the title of the target window."}
					}},
					{"state", {
						{"type", "string"},
						{"description", "The desired state. Must be one of: 'minimize', 'maximize', 'restore'."}
					}}
				}},
				{"required", {"window_title", "state"}}
			}}
		};
	}
	// 辅助函数，用于模拟一次虚拟按键的按下和抬起
	void SendVirtualKey(WORD vk) {
		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = vk;

		// 按下按键
		SendInput(1, &input, sizeof(INPUT));

		// 释放按键
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}


	std::string MediaControlTool::Execute(const nlohmann::json& args) {
		if (!args.contains("command")) {
			return "Error: Missing required argument 'command'.";
		}
		std::string command = args["command"].get<std::string>();
		std::transform(command.begin(), command.end(), command.begin(), ::tolower); // 命令不区分大小写

		WORD vk_code = 0;

		if (command == "play_pause") {
			vk_code = VK_MEDIA_PLAY_PAUSE;
		}
		else if (command == "next") {
			vk_code = VK_MEDIA_NEXT_TRACK;
		}
		else if (command == "previous") {
			vk_code = VK_MEDIA_PREV_TRACK;
		}
		else if (command == "stop") {
			vk_code = VK_MEDIA_STOP;
		}
		else if (command == "volume_up") {
			vk_code = VK_VOLUME_UP;
		}
		else if (command == "volume_down") {
			vk_code = VK_VOLUME_DOWN;
		}
		else if (command == "mute") {
			vk_code = VK_VOLUME_MUTE;
		}
		else {
			return "Error: Invalid command. Use 'play_pause', 'next', 'previous', 'stop', 'volume_up', 'volume_down', or 'mute'.";
		}

		SendVirtualKey(vk_code);

		return "Success: Media command '" + command + "' executed.";
	}

	nlohmann::json MediaControlTool::GetDefinition() const {
		return {
			{"name", "media_control"},
			{"description", "Controls system-wide media playback and volume by simulating media key presses."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"command", {
						{"type", "string"},
						{"description", "The media command to execute. Must be one of: 'play_pause', 'next', 'previous', 'stop', 'volume_up', 'volume_down', 'mute'."}
					}}
				}},
				{"required", {"command"}}
			}}
		};
	}

	//===========Shell Tools==========

	std::string ExecuteShellCommandTool::Execute(const nlohmann::json& args) {
		if (!args.contains("command")) {
			return "Error: Missing required argument 'command'.";
		}
		std::string command = args["command"].get<std::string>();

		// 为 CreateProcess 准备可写的命令行缓冲区
		std::vector<char> command_buffer(command.begin(), command.end());
		command_buffer.push_back('\0');

		HANDLE hChildStd_OUT_Rd = NULL;
		HANDLE hChildStd_OUT_Wr = NULL;
		SECURITY_ATTRIBUTES sa;

		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		sa.lpSecurityDescriptor = NULL;

		// 创建管道用于捕获子进程的输出
		if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &sa, 0)) {
			return "Error: Failed to create pipe for command output.";
		}

		// 确保读句柄不会被子进程继承
		if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
			CloseHandle(hChildStd_OUT_Rd);
			CloseHandle(hChildStd_OUT_Wr);
			return "Error: Failed to set handle information.";
		}

		PROCESS_INFORMATION piProcInfo;
		STARTUPINFOA siStartInfo;
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

		siStartInfo.cb = sizeof(STARTUPINFOA);
		// 重定向子进程的标准输出和标准错误到我们的管道
		siStartInfo.hStdError = hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = hChildStd_OUT_Wr;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		// 创建子进程来执行命令
		// 我们通过 cmd.exe /c 来执行命令，这能确保内置命令(如 'dir')也能正常工作
		std::string cmd_line = "cmd.exe /c " + command;
		std::vector<char> cmd_line_buffer(cmd_line.begin(), cmd_line.end());
		cmd_line_buffer.push_back('\0');

		bool bSuccess = CreateProcessA(NULL,
			cmd_line_buffer.data(), // 命令行
			NULL,           // 进程句柄不可继承
			NULL,           // 线程句柄不可继承
			TRUE,           // 设置句柄可继承
			CREATE_NO_WINDOW, // 不创建窗口
			NULL,           // 使用父进程的环境块
			NULL,           // 使用父进程的起始目录 
			&siStartInfo,   // 指向 STARTUPINFO 结构
			&piProcInfo);   // 指向 PROCESS_INFORMATION 结构

		// 必须在创建进程后关闭写句柄，否则 ReadFile 将永远阻塞
		CloseHandle(hChildStd_OUT_Wr);

		if (!bSuccess) {
			CloseHandle(hChildStd_OUT_Rd);
			return "Error: CreateProcess failed to execute command.";
		}

		// 读取管道中的输出
		std::string output = "";
		DWORD dwRead;
		CHAR chBuf[4096];
		while (ReadFile(hChildStd_OUT_Rd, chBuf, sizeof(chBuf), &dwRead, NULL) && dwRead != 0) {
			output.append(chBuf, dwRead);
		}

		// 等待子进程结束
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);

		// 清理资源
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		CloseHandle(hChildStd_OUT_Rd);

		return "Success: Command executed.\nOutput:\n" + output;
	}

	nlohmann::json ExecuteShellCommandTool::GetDefinition() const {
		return {
			{"name", "execute_shell_command"},
			{"description", "Executes a shell command (via cmd.exe) and returns its standard output and error. This is a powerful tool for system interaction. Use with caution."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"command", {
						{"type", "string"},
						{"description", "The command to execute, e.g., 'dir C:\\Users', 'ping google.com', 'echo Hello World'."}
					}}
				}},
				{"required", {"command"}}
			}}
		};
	}
	//===========Input Simulation Tools==========

// --- TypeTextTool 实现 ---
	std::string TypeTextTool::Execute(const nlohmann::json& args) {
		if (!args.contains("text")) {
			return "Error: Missing required argument 'text'.";
		}
		std::string text = args["text"].get<std::string>();

		// 将 UTF-8 字符串转换为 UTF-16 (wstring)
		if (text.empty()) {
			return "Success: Executed with empty text.";
		}
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);
		if (size_needed <= 0) {
			return "Error: Failed to convert text to wide string.";
		}
		std::wstring wtext(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &wtext[0], size_needed);

		std::vector<INPUT> inputs;
		for (wchar_t wc : wtext) {
			INPUT input_down = { 0 };
			input_down.type = INPUT_KEYBOARD;
			input_down.ki.wScan = wc;
			input_down.ki.dwFlags = KEYEVENTF_UNICODE;
			inputs.push_back(input_down);

			INPUT input_up = { 0 };
			input_up.type = INPUT_KEYBOARD;
			input_up.ki.wScan = wc;
			input_up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
			inputs.push_back(input_up);
		}

		if (!inputs.empty()) {
			SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
		}

		return "Success: Text typed.";
	}

	nlohmann::json TypeTextTool::GetDefinition() const {
		return {
			{"name", "type_text"},
			{"description", "Simulates typing a string of text. Supports Unicode characters."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"text", {
						{"type", "string"},
						{"description", "The text to be typed."}
					}}
				}},
				{"required", {"text"}}
			}}
		};
	}

	// --- PressKeysTool 实现 ---

	// 辅助函数：将字符串键名映射到虚拟键码
	WORD StringToVirtualKeyCode(const std::string& key) {
		static const std::map<std::string, WORD> keyMap = {
			// 修饰键
			{"control", VK_CONTROL}, {"ctrl", VK_CONTROL},
			{"shift", VK_SHIFT},
			{"alt", VK_MENU},
			{"win", VK_LWIN}, {"windows", VK_LWIN},
			// 功能键
			{"enter", VK_RETURN}, {"return", VK_RETURN},
			{"tab", VK_TAB},
			{"escape", VK_ESCAPE}, {"esc", VK_ESCAPE},
			{"backspace", VK_BACK},
			{"delete", VK_DELETE}, {"del", VK_DELETE},
			{"insert", VK_INSERT},
			{"home", VK_HOME},
			{"end", VK_END},
			{"pageup", VK_PRIOR},
			{"pagedown", VK_NEXT},
			// 方向键
			{"left", VK_LEFT},
			{"right", VK_RIGHT},
			{"up", VK_UP},
			{"down", VK_DOWN},
			// F功能键
			{"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4},
			{"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
			{"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12}
		};

		std::string lower_key = key;
		std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

		auto it = keyMap.find(lower_key);
		if (it != keyMap.end()) {
			return it->second;
		}

		// 处理单个字符
		if (lower_key.length() == 1) {
			char c = lower_key[0];
			if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
				return VkKeyScanA(c) & 0xFF;
			}
		}
		return 0; // 未找到
	}

	std::string PressKeysTool::Execute(const nlohmann::json& args) {
		if (!args.contains("keys") || !args["keys"].is_array()) {
			return "Error: Missing or invalid 'keys' argument. It must be an array of strings.";
		}

		std::vector<WORD> vkeys;
		for (const auto& key_str : args["keys"]) {
			WORD vk = StringToVirtualKeyCode(key_str.get<std::string>());
			if (vk == 0) {
				return "Error: Invalid key name provided: " + key_str.get<std::string>();
			}
			vkeys.push_back(vk);
		}

		if (vkeys.empty()) {
			return "Error: No valid keys to press.";
		}

		std::vector<INPUT> inputs;
		// 按下所有键
		for (WORD vk : vkeys) {
			INPUT input = { 0 };
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = vk;
			inputs.push_back(input);
		}
		// 以相反的顺序释放所有键
		for (auto it = vkeys.rbegin(); it != vkeys.rend(); ++it) {
			INPUT input = { 0 };
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = *it;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			inputs.push_back(input);
		}

		SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));

		return "Success: Keys pressed and released.";
	}


	nlohmann::json PressKeysTool::GetDefinition() const {
		return {
			{"name", "press_keys"},
			{"description", "Simulates pressing a combination of keys (keyboard shortcut). For example, to press Ctrl+C, provide ['control', 'c']."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"keys", {
						{"type", "array"},
						{"description", "An array of key names to press simultaneously. Common names: 'control', 'shift', 'alt', 'win', 'enter', 'tab', 'esc', 'a', 'b', 'c', 'f1', etc."},
						{"items", {{"type", "string"}}}
					}}
				}},
				{"required", {"keys"}}
			}}
		};
	}


	// --- MouseMoveTool 实现 ---
	std::string MouseMoveTool::Execute(const nlohmann::json& args) {
		if (!args.contains("x") || !args.contains("y")) {
			return "Error: Missing required arguments 'x' or 'y'.";
		}

		long x = args["x"].get<long>();
		long y = args["y"].get<long>();
		bool absolute = args.value("absolute", true);

		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		input.mi.dx = x;
		input.mi.dy = y;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;

		if (absolute) {
			int screen_width = GetSystemMetrics(SM_CXSCREEN);
			int screen_height = GetSystemMetrics(SM_CYSCREEN);
			input.mi.dx = (x * 65535) / screen_width;
			input.mi.dy = (y * 65535) / screen_height;
			input.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
		}

		SendInput(1, &input, sizeof(INPUT));
		return "Success: Mouse moved.";
	}

	nlohmann::json MouseMoveTool::GetDefinition() const {
		return {
			{"name", "move_mouse"},
			{"description", "Moves the mouse cursor to specific coordinates or by a relative amount."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"x", {
						{"type", "integer"},
						{"description", "The horizontal position (or offset if relative)."}
					}},
					{"y", {
						{"type", "integer"},
						{"description", "The vertical position (or offset if relative)."}
					}},
					{"absolute", {
						{"type", "boolean"},
						{"description", "If true (default), x and y are absolute screen coordinates. If false, they are relative offsets from the current position."}
					}}
				}},
				{"required", {"x", "y"}}
			}}
		};
	}


	// --- MouseClickTool 实现 ---
	std::string MouseClickTool::Execute(const nlohmann::json& args) {
		std::string button = args.value("button", "left");
		bool double_click = args.value("double_click", false);
		std::transform(button.begin(), button.end(), button.begin(), ::tolower);

		DWORD down_flag = 0, up_flag = 0;
		if (button == "left") {
			down_flag = MOUSEEVENTF_LEFTDOWN;
			up_flag = MOUSEEVENTF_LEFTUP;
		}
		else if (button == "right") {
			down_flag = MOUSEEVENTF_RIGHTDOWN;
			up_flag = MOUSEEVENTF_RIGHTUP;
		}
		else if (button == "middle") {
			down_flag = MOUSEEVENTF_MIDDLEDOWN;
			up_flag = MOUSEEVENTF_MIDDLEUP;
		}
		else {
			return "Error: Invalid button specified. Use 'left', 'right', or 'middle'.";
		}

		std::vector<INPUT> inputs;
		int click_count = double_click ? 2 : 1;

		for (int i = 0; i < click_count; ++i) {
			INPUT input_down = { 0 };
			input_down.type = INPUT_MOUSE;
			input_down.mi.dwFlags = down_flag;
			inputs.push_back(input_down);

			INPUT input_up = { 0 };
			input_up.type = INPUT_MOUSE;
			input_up.mi.dwFlags = up_flag;
			inputs.push_back(input_up);
		}

		SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));

		return "Success: Mouse click performed.";
	}

	nlohmann::json MouseClickTool::GetDefinition() const {
		return {
			{"name", "mouse_click"},
			{"description", "Simulates a mouse click."},
			{"parameters", {
				{"type", "object"},
				{"properties", {
					{"button", {
						{"type", "string"},
						{"description", "The button to click. 'left' (default), 'right', or 'middle'."}
					}},
					{"double_click", {
						{"type", "boolean"},
						{"description", "Set to true to perform a double-click. Default is false."}
					}}
				}},
				{"required", nlohmann::json::array()}
			}}
		};
	}

}