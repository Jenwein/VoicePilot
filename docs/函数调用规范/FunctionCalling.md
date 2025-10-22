// LLM阶段
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