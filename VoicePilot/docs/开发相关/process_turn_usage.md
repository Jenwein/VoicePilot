# Process Turn 功能使用说明

## 概述

新的 `process_turn` 功能是 ai_service.py 的核心重构，支持多轮对话和复杂任务的多工具调用。它统一了之前的 `understand` 和 `generate_response` 功能，实现了真正的对话循环。

## 命令格式

```bash
python ai_service.py process_turn --tool_definitions_path <工具定义文件> [--user_input_audio_path <音频文件>] [--tool_results_json <工具结果JSON>]
```

## 参数说明

- `--tool_definitions_path`: 必需，包含工具定义的JSON文件路径
- `--user_input_audio_path`: 可选，用户输入的音频文件路径（仅首轮对话需要）
- `--tool_results_json`: 可选，上一轮工具执行结果的JSON字符串（后续轮次需要）

## 使用场景

### 1. 首轮对话

当用户首次发起请求时，使用音频文件：

```bash
python ai_service.py process_turn \
  --tool_definitions_path "Resources/prompts/tools.json" \
  --user_input_audio_path "input.wav"
```

**返回格式：**
- 需要调用工具时：
```json
{
  "tool_calls": [
    {
      "name": "get_current_time",
      "args": {}
    },
    {
      "name": "get_known_folder_path",
      "args": {"folder_name": "desktop"}
    }
  ]
}
```

- 直接回复时：
```json
{
  "final_response": "好的，我已经理解了您的请求。"
}
```

### 2. 后续轮次

当C++执行完工具后，将结果反馈给Python：

```bash
python ai_service.py process_turn \
  --tool_definitions_path "Resources/prompts/tools.json" \
  --tool_results_json '[{"tool_name":"get_current_time","content":"Success: 2025-10-22 10:30:00"}]'
```

## 工具结果JSON格式

```json
[
  {
    "tool_name": "工具名称",
    "content": "执行结果内容"
  }
]
```

## 完整流程示例

假设用户说："获取当前时间，写入桌面文件"

### 第1轮 (C++ → Python → C++)
```bash
# C++调用
python ai_service.py process_turn \
  --tool_definitions_path "tools.json" \
  --user_input_audio_path "user_request.wav"

# Python返回
{
  "tool_calls": [
    {"name": "get_current_time", "args": {}},
    {"name": "get_known_folder_path", "args": {"folder_name": "desktop"}}
  ]
}

# C++执行工具，得到结果
```

### 第2轮 (C++ → Python → C++)
```bash
# C++调用
python ai_service.py process_turn \
  --tool_definitions_path "tools.json" \
  --tool_results_json '[
    {"tool_name":"get_current_time","content":"Success: 2025-10-22 10:30:00"},
    {"tool_name":"get_known_folder_path","content":"Success: C:/Users/User/Desktop"}
  ]'

# Python返回
{
  "tool_calls": [
    {
      "name": "write_to_file",
      "args": {
        "path": "C:/Users/User/Desktop/current_time.txt",
        "content": "当前时间：2025-10-22 10:30:00"
      }
    }
  ]
}

# C++执行write_to_file工具
```

### 第3轮 (C++ → Python → C++)
```bash
# C++调用
python ai_service.py process_turn \
  --tool_definitions_path "tools.json" \
  --tool_results_json '[
    {"tool_name":"write_to_file","content":"Success: 文件已写入 C:/Users/User/Desktop/current_time.txt"}
  ]'

# Python返回
{
  "final_response": "好的，我已经获取了当前时间并保存到您的桌面文件中。"
}

# C++收到final_response，调用TTS播报，流程结束
```

## 错误处理

当发生错误时，返回格式：
```json
{
  "error": "错误类型",
  "details": "详细错误信息"
}
```

## 兼容性

为了保持向后兼容，原有的 `understand` 和 `generate_response` 命令仍然可用，但内部会转换为 `process_turn` 调用。

## 技术实现要点

1. **对话历史管理**: 使用 `types.Part.from_function_response()` 构建完整的对话历史
2. **多工具支持**: 可以在单次响应中返回多个工具调用
3. **状态判断**: 根据模型响应自动判断是继续调用工具还是结束对话
4. **错误恢复**: 完善的异常处理和错误信息返回

这个重构为实现复杂的多步骤任务奠定了基础，支持真正的Agent式工作流程。