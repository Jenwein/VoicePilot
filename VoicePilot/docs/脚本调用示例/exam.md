```bash
❯ python scripts/ai_service.py ASR
INFO:root:收到音频转录请求: 文件路径=Resources\audios\input.wav
INFO:root:正在读取音频文件...
INFO:root:正在调用 Gemini ASR API...
INFO:google_genai.models:AFC is enabled with max remote calls: 10.
INFO:httpx:HTTP Request: POST https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent "HTTP/1.1 200 OK"
INFO:root:转录成功: 现在几点
{"status": "success", "transcript": "现在几点"}
```

```bash
❯ python scripts/ai_service.py LLM --user_request "现在几点了，桌面路径是什么" --tools_file "Resources/prompts/toolDefsPrompt.json"
INFO:root:收到理解请求: 用户请求="现在几点了，桌面路径是什么", tools_file="Resources/prompts/toolDefsPrompt.json", previous_turn="None"
INFO:root:加载了 3 个工具定义
INFO:root:正在调用 Gemini API...
INFO:google_genai.models:AFC is enabled with max remote calls: 10.
INFO:httpx:HTTP Request: POST https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent "HTTP/1.1 200 OK"
INFO:root:检测到工具调用: get_current_time
INFO:root:检测到工具调用: get_known_folder_path
INFO:root:返回结果: status=continue, function_calls=2
{
  "status": "continue",
  "function_calls": [
    {
      "name": "get_current_time",
      "args": {}
    },
    {
      "name": "get_known_folder_path",
      "args": {
        "folder_name": "Desktop"
      }
    }
  ],
  "response_text": "",
  "reasoning": "需要执行工具调用"
}
```

```bash
❯ python scripts/ai_service.py TTS --text "现在你真是大帅哥"                                                           
INFO:root:收到TTS任务: 文本="现在你真是大帅哥", 输出到="Resources\audios\output.wav"
INFO:root:正在向 Gemini TTS API 发送请求...
INFO:google_genai.models:AFC is enabled with max remote calls: 10.
INFO:httpx:HTTP Request: POST https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-preview-tts:generateContent "HTTP/1.1 200 OK"
INFO:root:TTS成功，文件保存到: Resources\audios\output.wav
{"status": "success", "message": "语音文件已成功保存到: Resources\\audios\\output.wav"}
```