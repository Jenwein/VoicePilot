#include "../Tools/SystemTools.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h> 
#pragma comment(lib, "Shell32.lib")
#endif

namespace Razel {

	std::string getCurrentTimeString() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm now_tm;

		// Use localtime_s for thread safety on Windows, or localtime_r on POSIX
#ifdef _WIN32
		localtime_s(&now_tm, &now_c);
#else
		localtime_r(&now_c, &now_tm);
#endif

		std::stringstream ss;
		ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	std::string GetCurrentTimeTool::Execute(const nlohmann::json& args) {
		// 这个工具忽略任何传入的参数
		return "Success: Current time is " + getCurrentTimeString();
	}

	nlohmann::json GetCurrentTimeTool::GetDefinition() const {
		return {
			{"name", "get_current_time"},
			{"description", "Gets the current local date and time."},
			{"parameters", {
				{"type", "OBJECT"},
				{"properties", {}},
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

			// 确保目录存在
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
				{"type", "OBJECT"},
				{"properties", {
					{"path", {
						{"type", "STRING"},
						{"description", "The full path of the file to write to, e.g., 'C:/Users/User/Desktop/example.txt'"}
					}},
					{"content", {
						{"type", "STRING"},
						{"description", "The text content to write to the file."}
					}}
				}},
				{"required", {"path", "content"}}
			}}
		};
	}

	// --- GetKnownFolderPathTool 实现 ---
	std::string GetKnownFolderPathTool::Execute(const nlohmann::json& args) {
		if (!args.contains("folder_name")) {
			return "Error: Missing required argument 'folder_name'.";
		}
		std::string folderName = args["folder_name"].get<std::string>();

#ifdef _WIN32
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
#else
		// 为 Linux/macOS 提供一个简单的实现
		const char* home = getenv("HOME");
		if (home == nullptr) {
			return "Error: Could not get HOME environment variable.";
		}
		std::string path(home);
		if (folderName == "desktop") {
			path += "/Desktop";
		}
		else if (folderName == "documents") {
			path += "/Documents";
		}
		else if (folderName == "downloads") {
			path += "/Downloads";
		}
		else {
			return "Error: Unsupported folder name. Use 'desktop', 'documents', or 'downloads'.";
		}
		return "Success: " + path;
#endif
	}

	nlohmann::json GetKnownFolderPathTool::GetDefinition() const {
		return {
			{"name", "get_known_folder_path"},
			{"description", "Gets the absolute path of a standard system folder."},
			{"parameters", {
				{"type", "OBJECT"},
				{"properties", {
					{"folder_name", {
						{"type", "STRING"},
						{"description", "The name of the folder to query. Supported values: 'desktop', 'documents', 'downloads'."}
					}}
				}},
				{"required", {"folder_name"}}
			}}
		};
	}

} // namespace Razel