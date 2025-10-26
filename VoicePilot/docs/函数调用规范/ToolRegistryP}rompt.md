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
### 单个工具调用（向后兼容）
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

### 多个工具顺序调用
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

### 支持依赖关系的工具调用
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

### 错误返回
```json
{
  "error": "错误描述",
  "details": "详细错误信息"
}
```

## 响应生成阶段返回的json格式
```json
{
  "response": "自然语言回复文本"
}
```

## 语音合成阶段返回的json格式
### 成功
```json
{
  "status": "success",
  "message": "语音文件已成功保存到: output.wav"
}
```

### 错误
```json
{
  "error": "错误描述",
  "details": "详细错误信息"
}
```