# VoicePilot

一个桌面语音助手应用程序，支持语音识别、自然语言处理和智能响应。

视频链接:https://www.bilibili.com/video/BV1oPszzJEM8/?vd_source=2e3dafb0c18640e83790e924b40c44fb

**文档架构等说明请见根下docs目录**

## 🚀 快速开始

### 环境要求

- **编译器**: Visual Studio 2022+ (Windows)
- **构建工具**: Premake5
- **依赖管理**: Git 子模块
- **操作系统**: Windows 10/11

### 编译步骤

1. **克隆仓库**
   ```bash
   git clone https://github.com/Jenwein/VoicePilot.git
   cd VoicePilot
   ```

2. **初始化子模块**
   ```bash
   git submodule update --init --recursive
   ```

3. **生成项目文件**
   ```bash
   # Windows (生成 Visual Studio 解决方案)
   ./scripts/Win-Generation.bat
   ```

4. **编译项目**
   ```bash
   # Windows
   # 打开生成的 .sln 文件，在 Visual Studio 中编译
   ```

   可能需要补充Python3.12环境,并 `pip install -q -U google-genai`

## 🎮 使用说明

### 基本使用

1. 启动 VoicePilot 应用程序
2. 允许麦克风权限（首次使用）
3. 点击3D兔子开始语音输入,再次点击发送请求
4. 说出您的指令或问题
5. 语音助手的响应


## ✨ 功能特性

- 🎤 **实时语音识别** - 高精度的语音转文字功能
- 🧠 **智能对话** - 支持自然语言理解和响应
- 🔧 **可扩展架构** - 模块化设计，易于添加新功能

### 配置选项

在 `config/` 目录下可以找到配置文件：

- `audio.json` - 音频设备和处理参数
- `ai.json` - AI 模型和响应设置
- `ui.json` - 用户界面主题和布局

## 🔧 技术栈

- **核心语言**: C++17，Python
- **图形渲染**: OpenGL + ImGui
- **音频处理**: miniaudio
- **语音识别**: Gemini ASR Api
- **构建系统**: Premake5
- **版本控制**: Git + Git 子模块

## 📄 许可证

本项目采用 [MIT 许可证](LICENSE) - 查看 LICENSE 文件了解详情。

## 📞 联系方式

- **作者**: Jenwein
- **GitHub**: [@Jenwein](https://github.com/Jenwein)
- **邮箱**: [rgw127310@gmail.com,1273106078@qq.com]

## 开源使用

- [ImGui](https://github.com/ocornut/imgui) - 即时模式图形用户界面
---

⭐ 如果这个项目对您有帮助，请给个 Star！
