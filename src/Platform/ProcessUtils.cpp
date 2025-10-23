#include "ProcessUtils.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <windows.h>
#include <locale>
#include <codecvt>

namespace Razel {

	// UTF-8 到 UTF-16 转换
	std::wstring Utf8ToUtf16(const std::string& utf8) {
		if (utf8.empty()) return std::wstring();

		int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		if (size == 0) return std::wstring();

		std::wstring utf16(size - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], size);
		return utf16;
	}

	// UTF-16 到 UTF-8 转换
	std::string Utf16ToUtf8(const std::wstring& utf16) {
		if (utf16.empty()) return std::string();

		int size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (size == 0) return std::string();

		std::string utf8(size - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], size, nullptr, nullptr);
		return utf8;
	}

	std::string ProcessUtils::Exec(const char* cmd) {
		// 转换为宽字符
		std::wstring wcmd = Utf8ToUtf16(std::string(cmd));

		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		HANDLE hChildStd_OUT_Rd = NULL;
		HANDLE hChildStd_OUT_Wr = NULL;

		// 创建管道
		if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
			throw std::runtime_error("Failed to create pipe!");
		}

		// 确保读取句柄不被子进程继承
		if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
			CloseHandle(hChildStd_OUT_Rd);
			CloseHandle(hChildStd_OUT_Wr);
			throw std::runtime_error("Failed to set handle information!");
		}

		PROCESS_INFORMATION piProcInfo;
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		STARTUPINFOW siStartInfo;
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFOW));
		siStartInfo.cb = sizeof(STARTUPINFOW);
		siStartInfo.hStdError = hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = hChildStd_OUT_Wr;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		// 创建子进程
		BOOL bSuccess = CreateProcessW(NULL,
			const_cast<LPWSTR>(wcmd.c_str()),
			NULL,
			NULL,
			TRUE,
			CREATE_NO_WINDOW,  // 不创建窗口
			NULL,
			NULL,
			&siStartInfo,
			&piProcInfo);

		if (!bSuccess) {
			CloseHandle(hChildStd_OUT_Rd);
			CloseHandle(hChildStd_OUT_Wr);
			throw std::runtime_error("Failed to create process!");
		}

		// 关闭写入句柄，这样子进程结束时管道会关闭
		CloseHandle(hChildStd_OUT_Wr);

		// 读取输出
		std::string result;
		DWORD dwRead;
		CHAR chBuf[4096];

		while (ReadFile(hChildStd_OUT_Rd, chBuf, 4096, &dwRead, NULL) && dwRead > 0) {
			result.append(chBuf, dwRead);
		}

		// 等待子进程结束
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);

		// 清理句柄
		CloseHandle(hChildStd_OUT_Rd);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);

		return result;
	}
}