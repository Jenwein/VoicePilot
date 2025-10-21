## 工具函数注册json格式(提供给ASR的Prompt)

```json
{
  "name": "工具名称",
  "description": "工具描述",
  "parameters": {
    "type": "OBJECT",
    "properties": {
      "参数名1": {
        "type": "参数类型",
        "description": "参数描述"
      },
      "参数名2": {
        "type": "参数类型",
        "description": "参数描述"
      }
    },
    "required": ["必需参数名1", "必需参数名2"]
  }
}
```

## ASR返回的json格式
### 成功返回
```json
{
  "functionCall": {
    "name": "工具名称",
    "args": {
      "参数名1": "参数值1",
      "参数名2": "参数值2"
    }
  }
}
```
### 错误返回
```json
{
  "error": "错误描述",
  "details": "详细错误信息"
}
```

### 扩展为多工具调用,支持单个,多个顺序,多个依赖

```json
{
  "functionCall": {
    "name": "工具名称",
    "args": {
      "参数名1": "参数值1",
      "参数名2": "参数值2"
    }
  }
}
```

```json
{
  "functionCalls": [
    {
      "id": 1,
      "name": "工具名称1",
      "args": {
        "参数名1": "参数值1"
      }
    },
    {
      "id": 2,
      "name": "工具名称2",
      "args": {
        "参数名1": "参数值1",
        "参数名2": "参数值2"
      }
    }
  ]
}
```

```json
{
  "functionCalls": [
    {
      "id": 1,
      "name": "工具名称1",
      "args": {
        "参数名1": "参数值1"
      }
    },
    {
      "id": 2,
      "name": "工具名称2",
      "args": {
        "参数名1": "{1.参数名1}",
        "参数名2": "参数值2"
      },
      "dependsOn": [1]
    }
  ]
}
```

您说得对，让我们继续梳理第三和第四阶段的问题。让我分析一下目前存在的不匹配问题：

## 第三阶段（响应生成）和第四阶段（语音合成）的不匹配问题

### 1. 第三阶段的问题

**C++端期望的行为：**
- C++调用Python的`generate_response`子命令
- 传递工具执行结果给Python
- 期望Python返回一个自然语言回复

**Python端实际行为：**
- Python接收工具执行结果
- 生成自然语言回复
- 直接打印回复文本到标准输出

**不匹配点：**
- C++端试图从Python输出中提取JSON格式的内容：
```cpp
std::string jsonOutput;
size_t firstBrace = output.find('{');
size_t lastBrace = output.rfind('}');

if (firstBrace != std::string::npos && lastBrace != std::string::npos && firstBrace < lastBrace) {
    jsonOutput = output.substr(firstBrace, lastBrace - firstBrace + 1);
}
else {
    // 没有找到有效的JSON结构时使用原始输出
    jsonOutput = output;
}
```
- 但实际上Python的`generate_response`函数只是直接打印自然语言文本，不是JSON格式

### 2. 第四阶段的问题

**C++端期望的行为：**
- C++调用Python的`tts`子命令
- 传递自然语言文本给Python
- Python生成语音文件

**Python端实际行为：**
- Python接收文本和输出文件路径
- 生成语音并保存到文件
- 打印成功信息到标准输出

**不匹配点：**
- C++端将第三阶段提取的"JSON"（实际上是自然语言文本）作为`tts`的文本参数传递：
```cpp
std::string finalResponseText = jsonOutput;  // 这里是自然语言文本，不是JSON

PythonScriptCommand ttsCommand;
ttsCommand.SubCommand = "tts";
ttsCommand.Args = {
    {"--text", finalResponseText},  // 传递的是自然语言文本
    {"--output_file", m_OutputAudioPath}
};
```

### 3. 具体的不匹配示例

假设工具执行结果是："Success: Content successfully written to C:/Users/User/Desktop/note.txt"

**第三阶段流程：**
1. C++调用Python `generate_response`，传递结果
2. Python生成回复："好的，我已经将内容写入到您的桌面note.txt文件中了。"
3. Python直接打印这个文本到标准输出
4. C++尝试从输出中提取JSON，但找不到大括号，所以`jsonOutput`就是原始文本
5. C++将这个文本当作JSON传递给第四阶段

**第四阶段流程：**
1. C++调用Python `tts`，传递"好的，我已经将内容写入到您的桌面note.txt文件中了。"作为文本
2. Python接收文本并生成语音
3. Python打印成功信息到标准输出
4. C++不处理这个输出，直接播放生成的音频文件

### 4. 主要问题总结

1. **第三阶段JSON提取逻辑错误**：C++试图从非JSON输出中提取JSON
2. **变量命名误导**：`jsonOutput`实际上不是JSON
3. **处理逻辑混乱**：第三阶段和第四阶段之间的数据传递没有清晰的边界

这些不匹配导致了两个阶段之间无法正确解析和传递数据。