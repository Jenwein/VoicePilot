//工具函数调用格式要求(定义函数声明):
```json
{
    "name": "set_light_values",
    "description": "Sets the brightness and color temperature of a light.",
    "parameters": {
        "type": "object",
        "properties": {
            "brightness": {
                "type": "integer",
                "description": "Light level from 0 to 100. Zero is off and 100 is full brightness",
            },
            "color_temp": {
                "type": "string",
                "enum": ["daylight", "cool", "warm"],
                "description": "Color temperature of the light fixture, which can be `daylight`, `cool` or `warm`.",
            },
        },
        "required": ["brightness", "color_temp"],
    },
```

// LLM阶段函数调用格式要求:
```json
// 输入格式
{
  "user_request": "用户请求文本",
  "tools_file": "工具定义文件路径",
  "previous_turn": {
    "function_calls": [...],
    "execution_results": [...]
  }
}
```
```json
// 输出格式
{
  "status": "continue" | "finished",
  "function_calls": [
    {
      "name": "function_name1",
      "args": {
        "param1": "value1",
        "param2": "value2"
      }
    },
    {
      "name": "function_name2",
      "args": {
        "param1": "value1",
        "param2": "value2"
      }
    }
  ],
  "response_text": "给用户的回复文本（仅在status为finished时有值）",
  "reasoning": "本轮推理过程说明（可选，用于调试）"
}
```