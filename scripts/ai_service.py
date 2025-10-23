# ai_service.py - 修改为支持pybind11调用的版本
import os
import json
import wave
import logging
from google import genai
from google.genai import types

# --- 1. 日志配置 ---
def setup_logging():
    """配置日志输出到文件，保持函数调用纯净"""
    # 创建logs目录（如果不存在）
    logs_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "logs")
    os.makedirs(logs_dir, exist_ok=True)
    
    # 配置日志文件路径
    log_file = os.path.join(logs_dir, "ai_service.log")
    
    # 配置日志格式和输出
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_file, encoding='utf-8'),
        ]
    )
    
    # 禁用所有其他库的控制台日志输出
    logging.getLogger().handlers = [h for h in logging.getLogger().handlers if not isinstance(h, logging.StreamHandler)]

# 在模块加载时立即设置日志
setup_logging()

# --- 2. 配置 ---
def _get_client():
    """获取Gemini客户端，如果失败抛出异常"""
    try:
        return genai.Client()
    except KeyError:
        logging.error("请先设置 GEMINI_API_KEY 环境变量。")
        raise RuntimeError("环境变量未设置：请先设置 GEMINI_API_KEY 环境变量")

# --- 3. 核心功能函数 ---

def transcribe_audio(audio_file_path: str = None) -> dict:
    """
    音频转录函数
    
    Args:
        audio_file_path (str, optional): 音频文件路径，默认为 Resources/audios/input.wav
    
    Returns:
        dict: 包含转录结果的字典
            成功: {"status": "success", "transcript": "转录文本"}
            失败: {"error": "错误类型", "details": "错误详情"}
    """
    if audio_file_path is None:
        audio_file_path = os.path.join("Resources", "audios", "input.wav")
    
    logging.info(f'收到音频转录请求: 文件路径={audio_file_path}')
    
    try:
        # 检查音频文件是否存在
        if not os.path.exists(audio_file_path):
            error_msg = f"音频文件未找到: {audio_file_path}"
            logging.error(error_msg)
            return {
                "error": "音频文件未找到",
                "details": f"文件路径: {audio_file_path}"
            }
        
        # 读取音频文件并转换为字节数据
        logging.info("正在读取音频文件...")
        with open(audio_file_path, 'rb') as f:
            audio_bytes = f.read()
        
        # 获取客户端并调用 Gemini API 进行音频转录
        client = _get_client()
        logging.info("正在调用 Gemini ASR API...")
        response = client.models.generate_content(
            model='gemini-2.5-flash',
            contents=[
                'Transcribe the speech to plain Simplified Chinese text. Output only the transcribed text, without any explanations, tags, or formatting.',
                types.Part.from_bytes(
                    data=audio_bytes,
                    mime_type='audio/wav',
                )
            ]
        )
        
        # 提取转录文本
        if response.text:
            transcript = response.text.strip()
            logging.info(f"转录成功: {transcript}")
            return {
                "status": "success",
                "transcript": transcript
            }
        else:
            error_msg = "API未返回转录文本"
            logging.error(error_msg)
            return {
                "error": "转录失败",
                "details": error_msg
            }
            
    except Exception as e:
        error_msg = f"音频转录失败: {str(e)}"
        logging.error(error_msg)
        return {
            "error": "音频转录失败",
            "details": str(e)
        }

def process_user_request(user_request: str, tools_file: str, previous_turn: str = None) -> dict:
    """
    处理用户请求并规划操作
    
    Args:
        user_request (str): 用户请求文本
        tools_file (str): 包含Tools定义的JSON文件路径
        previous_turn (str, optional): 上一轮操作JSON字符串
    
    Returns:
        dict: 包含处理结果的字典
            成功: {"status": "continue/finished", "function_calls": [...], "response_text": "...", "reasoning": "..."}
            失败: {"error": "错误类型", "details": "错误详情"}
    """
    logging.info(f'收到理解请求: 用户请求="{user_request}", tools_file="{tools_file}", previous_turn="{previous_turn}"')
    
    try:
        # 1. 加载工具定义
        tools_definitions = []
        if tools_file and os.path.exists(tools_file):
            with open(tools_file, 'r', encoding='utf-8') as f:
                tools_data = json.load(f)
                tools_definitions = tools_data.get('tools', [])
                logging.info(f"加载了 {len(tools_definitions)} 个工具定义")
        
        # 2. 解析上一轮的结果
        previous_turn_data = None
        if previous_turn:
            try:
                previous_turn_data = json.loads(previous_turn)
                logging.info("解析了上一轮操作结果")
            except json.JSONDecodeError as e:
                logging.warning(f"无法解析上一轮操作JSON: {e}")
        
        # 3. 构建对话内容
        contents = []
        
        # 如果是第一轮，添加用户请求
        if not previous_turn_data:
            contents.append(types.Content(
                role="user", 
                parts=[types.Part(text=user_request)]
            ))
        else:
            # 如果有上一轮的结果，需要构建完整的对话历史
            # 先添加原始用户请求
            contents.append(types.Content(
                role="user", 
                parts=[types.Part(text=user_request)]
            ))
            
            # 添加之前的模型响应（包含function_call）
            if previous_turn_data.get('function_calls'):
                model_parts = []
                for func_call in previous_turn_data['function_calls']:
                    model_parts.append(types.Part(
                        function_call=types.FunctionCall(
                            name=func_call['name'],
                            args=func_call['args']
                        )
                    ))
                contents.append(types.Content(role="model", parts=model_parts))
                
                # 添加工具执行结果
                if previous_turn_data.get('execution_results'):
                    user_parts = []
                    for i, result in enumerate(previous_turn_data['execution_results']):
                        func_name = previous_turn_data['function_calls'][i]['name']
                        user_parts.append(types.Part.from_function_response(
                            name=func_name,
                            response=result
                        ))
                    contents.append(types.Content(role="user", parts=user_parts))
        
        # 4. 配置工具
        tools = None
        config = None
        if tools_definitions:
            # 转换工具定义格式为 Gemini API 格式
            function_declarations = []
            for tool in tools_definitions:
                function_declarations.append({
                    "name": tool["name"],
                    "description": tool["description"],
                    "parameters": tool["parameters"]
                })
            
            tools = types.Tool(function_declarations=function_declarations)
            config = types.GenerateContentConfig(tools=[tools])
        
        # 5. 调用 Gemini API
        client = _get_client()
        logging.info("正在调用 Gemini API...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=contents,
            config=config
        )
        
        # 6. 解析响应
        result = {
            "status": "finished",
            "function_calls": [],
            "response_text": "",
            "reasoning": ""
        }
        
        # 检查是否有 function call
        if (response.candidates and 
            response.candidates[0].content.parts):
            
            has_function_calls = False
            for part in response.candidates[0].content.parts:
                if hasattr(part, 'function_call') and part.function_call:
                    has_function_calls = True
                    result["function_calls"].append({
                        "name": part.function_call.name,
                        "args": dict(part.function_call.args)
                    })
                    logging.info(f"检测到工具调用: {part.function_call.name}")
            
            # 如果有工具调用，状态设为 continue
            if has_function_calls:
                result["status"] = "continue"
                result["reasoning"] = "需要执行工具调用"
            else:
                # 没有工具调用，提取文本响应
                if response.text:
                    result["response_text"] = response.text.strip()
                    result["reasoning"] = "对话结束，返回最终回复"
                else:
                    result["response_text"] = "我理解了您的请求。"
                    result["reasoning"] = "未获取到文本响应，使用默认回复"
        else:
            # 没有有效响应
            result["response_text"] = "抱歉，我无法理解您的请求。"
            result["reasoning"] = "API响应为空或无效"
        
        # 7. 返回结果
        logging.info(f"返回结果: status={result['status']}, function_calls={len(result['function_calls'])}")
        return result
        
    except FileNotFoundError:
        error_msg = f"工具定义文件未找到: {tools_file}"
        logging.error(error_msg)
        return {
            "error": "工具定义文件未找到",
            "details": f"文件路径: {tools_file}"
        }
        
    except json.JSONDecodeError as e:
        error_msg = f"JSON解析失败: {str(e)}"
        logging.error(error_msg)
        return {
            "error": "JSON解析失败",
            "details": str(e)
        }
        
    except Exception as e:
        error_msg = f"LLM处理失败: {str(e)}"
        logging.error(error_msg)
        return {
            "error": "LLM处理失败",
            "details": str(e)
        }

def _write_wave_file(filename: str, pcm_data: bytes, channels: int = 1, sample_width: int = 2, rate: int = 24000):
    """将原始PCM数据写入WAV文件"""
    with wave.open(filename, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(rate)
        wf.writeframes(pcm_data)

def synthesize_speech(text: str, output_file_path: str = None) -> dict:
    """
    文本转语音函数
    
    Args:
        text (str): 需要转换为语音的文本
        output_file_path (str, optional): 输出音频文件路径，默认为 Resources/audios/output.wav
    
    Returns:
        dict: 包含合成结果的字典
            成功: {"status": "success", "message": "语音文件已成功保存到: path"}
            失败: {"error": "错误类型", "details": "错误详情"}
    """
    if output_file_path is None:
        output_file_path = os.path.join("Resources", "audios", "output.wav")
    
    logging.info(f'收到TTS任务: 文本="{text}", 输出到="{output_file_path}"')

    try:
        # 确保输出目录存在
        os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
        
        # 1. 调用 TTS 模型
        client = _get_client()
        logging.info("正在向 Gemini TTS API 发送请求...")
        response = client.models.generate_content(
           model="gemini-2.5-flash-preview-tts",
           contents=text,
           config=types.GenerateContentConfig(
              response_modalities=["AUDIO"],
              speech_config=types.SpeechConfig(
                 voice_config=types.VoiceConfig(
                    prebuilt_voice_config=types.PrebuiltVoiceConfig(
                       voice_name='Kore',
                    )
                 )
              ),
           )
        )

        # 2. 提取音频数据
        if response.candidates and response.candidates[0].content.parts and response.candidates[0].content.parts[0].inline_data:
            audio_data = response.candidates[0].content.parts[0].inline_data.data

            # 3. 将音频数据写入文件
            _write_wave_file(output_file_path, audio_data)
            logging.info(f"TTS成功，文件保存到: {output_file_path}")
            
            return {
                "status": "success",
                "message": f"语音文件已成功保存到: {output_file_path}"
            }
        else:
            # 如果API没有返回预期的音频数据
            error_msg = "API响应中未找到有效的音频数据。"
            logging.error(error_msg)
            return {
                "error": "语音合成失败",
                "details": error_msg
            }

    except Exception as e:
        # 捕获API调用异常或文件写入异常
        error_msg = f"语音合成失败: {str(e)}"
        logging.error(error_msg)
        return {
            "error": "语音合成失败",
            "details": str(e)
        }

# --- 4. 测试函数 (已注释，供pybind11调用时使用) ---  
# def main():
#     """测试函数，验证三个核心功能"""
#     print("=== AI Service 功能测试 ===\n")
    
#     # 测试1: ASR - 音频转录
#     print("1. 测试音频转录 (ASR):")
#     print("-" * 40)
#     asr_result = transcribe_audio()
#     print(f"ASR 结果: {json.dumps(asr_result, ensure_ascii=False, indent=2)}")
#     print()
    
#     # 测试2: LLM - 用户请求处理
#     print("2. 测试用户请求处理 (LLM):")
#     print("-" * 40)
#     user_request = "现在几点了，桌面路径是什么"
#     tools_file = "Resources/prompts/toolDefsPrompt.json"
#     llm_result = process_user_request(user_request, tools_file)
#     print(f"LLM 结果: {json.dumps(llm_result, ensure_ascii=False, indent=2)}")
#     print()
    
#     # 测试3: TTS - 语音合成
#     print("3. 测试语音合成 (TTS):")
#     print("-" * 40)
#     text = "现在你真是大帅哥"
#     tts_result = synthesize_speech(text)
#     print(f"TTS 结果: {json.dumps(tts_result, ensure_ascii=False, indent=2)}")
#     print()
    
#     print("=== 测试完成 ===")
    
#     # 总结测试结果
#     print("\n测试总结:")
#     print(f"- ASR: {'✓ 成功' if 'status' in asr_result and asr_result['status'] == 'success' else '✗ 失败'}")
#     print(f"- LLM: {'✓ 成功' if 'status' in llm_result else '✗ 失败'}")
#     print(f"- TTS: {'✓ 成功' if 'status' in tts_result and tts_result['status'] == 'success' else '✗ 失败'}")


# if __name__ == "__main__":
#     main()