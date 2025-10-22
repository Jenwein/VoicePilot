# ai_service.py template
import os
import argparse
import json
import wave
import logging
import sys
from google import genai
from google.genai import types

# --- 1. 配置 ---
# 配置日志记录
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('ai_service.log', encoding='utf-8'),
        logging.StreamHandler(sys.stderr)  # 错误信息仍然输出到stderr
    ]
)

try:
    client = genai.Client()
except KeyError:
    logging.error("请先设置 GEMINI_API_KEY 环境变量。")
    exit(1)

# 临时方案，使用gemini的ASR，后期考虑在C++中使用离线STT/ASR
# 无参数,音频文件路径始终在assets/audios/下名称问input.wav,返回转录文本作为用户请求
def handle_audio():
    audio_file_path = os.path.join("assets", "audios", "input.wav")
    logging.info(f'收到音频转录请求: 文件路径={audio_file_path}')
    
    try:
        # 检查音频文件是否存在
        if not os.path.exists(audio_file_path):
            error_result = {
                "error": "音频文件未找到",
                "details": f"文件路径: {audio_file_path}"
            }
            logging.error(f"音频文件未找到: {audio_file_path}")
            print(json.dumps(error_result, ensure_ascii=False))
            exit(1)
        
        # 读取音频文件并转换为字节数据
        logging.info("正在读取音频文件...")
        with open(audio_file_path, 'rb') as f:
            audio_bytes = f.read()
        
        # 调用 Gemini API 进行音频转录
        logging.info("正在调用 Gemini ASR API...")
        response = client.models.generate_content(
            model='gemini-2.5-flash',
            contents=[
                'Generate a transcript of the speech.',
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
            
            # 直接输出转录文本（不使用JSON格式）
            print(transcript)
        else:
            error_result = {
                "error": "转录失败",
                "details": "API未返回转录文本"
            }
            logging.error("API未返回转录文本")
            print(json.dumps(error_result, ensure_ascii=False))
            exit(1)
            
    except FileNotFoundError:
        error_result = {
            "error": "音频文件未找到",
            "details": f"文件路径: {audio_file_path}"
        }
        logging.error(f"音频文件未找到: {audio_file_path}")
        print(json.dumps(error_result, ensure_ascii=False))
        exit(1)
        
    except Exception as e:
        error_result = {
            "error": "音频转录失败",
            "details": str(e)
        }
        logging.error(f"音频转录失败: {str(e)}")
        print(json.dumps(error_result, ensure_ascii=False))
        exit(1)

        
# 循环理解用户意图并规划操作，每轮接受用户请求文本，工具定义文件路径，上一轮操作JSON与C++执行结果，返回规划的操作JSON
def handle_process_turn(args):
    logging.info(f'收到理解请求: 用户请求="{args.user_request}", tools_file="{args.tools_file}", previous_turn="{args.previous_turn}"')
    
    try:
        # 1. 加载工具定义
        tools_definitions = []
        if args.tools_file and os.path.exists(args.tools_file):
            with open(args.tools_file, 'r', encoding='utf-8') as f:
                tools_data = json.load(f)
                tools_definitions = tools_data.get('tools', [])
                logging.info(f"加载了 {len(tools_definitions)} 个工具定义")
        
        # 2. 解析上一轮的结果
        previous_turn_data = None
        if args.previous_turn:
            try:
                previous_turn_data = json.loads(args.previous_turn)
                logging.info("解析了上一轮操作结果")
            except json.JSONDecodeError as e:
                logging.warning(f"无法解析上一轮操作JSON: {e}")
        
        # 3. 构建对话内容
        contents = []
        
        # 如果是第一轮，添加用户请求
        if not previous_turn_data:
            contents.append(types.Content(
                role="user", 
                parts=[types.Part(text=args.user_request)]
            ))
        else:
            # 如果有上一轮的结果，需要构建完整的对话历史
            # 先添加原始用户请求
            contents.append(types.Content(
                role="user", 
                parts=[types.Part(text=args.user_request)]
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
        print(json.dumps(result, ensure_ascii=False, indent=2))
        
    except FileNotFoundError:
        error_result = {
            "error": "工具定义文件未找到",
            "details": f"文件路径: {args.tools_file}"
        }
        logging.error(f"工具定义文件未找到: {args.tools_file}")
        print(json.dumps(error_result, ensure_ascii=False))
        exit(1)
        
    except json.JSONDecodeError as e:
        error_result = {
            "error": "JSON解析失败",
            "details": str(e)
        }
        logging.error(f"JSON解析失败: {str(e)}")
        print(json.dumps(error_result, ensure_ascii=False))
        exit(1)
        
    except Exception as e:
        error_result = {
            "error": "LLM处理失败",
            "details": str(e)
        }
        logging.error(f"LLM处理失败: {str(e)}")
        print(json.dumps(error_result, ensure_ascii=False))
        exit(1)

# 语音合成后续再测试,输出文件路径始终为assets/audios/output.wav
def write_wave_file(filename: str, pcm_data: bytes, channels: int = 1, sample_width: int = 2, rate: int = 24000):
    """将原始PCM数据写入WAV文件"""
    with wave.open(filename, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(rate)
        wf.writeframes(pcm_data)

def handle_tts(args):
    """
    处理"语音合成"任务 (Stage 4)
    接收文本，保存为音频文件
    """
    audio_file_path = os.path.join("assets", "audios", "output.wav")
    logging.info(f'收到TTS任务: 文本="{args.text}", 输出到="{audio_file_path}"')

    try:
        # 1. 调用 TTS 模型
        #    参考 TTSexam.py 和 Submodules - Google Gen AI SDK documentation.pdf
        #    API 参考: Page 189 (GenerateContentConfig), Page 399 (SpeechConfig)
        logging.info("正在向 Gemini TTS API 发送请求...")
        response = client.models.generate_content(
           model="gemini-2.5-flash-preview-tts", # 使用最新的TTS模型
           contents=args.text,
           config=types.GenerateContentConfig(
              response_modalities=["AUDIO"],
              speech_config=types.SpeechConfig(
                 voice_config=types.VoiceConfig(
                    prebuilt_voice_config=types.PrebuiltVoiceConfig(
                       # 您可以在这里选择不同的声音，'Kore' 是一个听起来不错的选择
                       voice_name='Kore',
                    )
                 )
              ),
           )
        )

        # 2. 提取音频数据
        #    数据位于 response.candidates[0].content.parts[0].inline_data.data
        if response.candidates and response.candidates[0].content.parts and response.candidates[0].content.parts[0].inline_data:
            audio_data = response.candidates[0].content.parts[0].inline_data.data

            # 3. 将音频数据写入文件
            write_wave_file(audio_file_path, audio_data)
            # 以JSON格式返回成功信息
            result = {
                "status": "success",
                "message": f"语音文件已成功保存到: {audio_file_path}"
            }
            print(json.dumps(result, ensure_ascii=False))
        else:
            # 如果API没有返回预期的音频数据
            error_msg = "API响应中未找到有效的音频数据。"
            logging.error(error_msg)
            raise ValueError(error_msg)

    except Exception as e:
        # 捕获API调用异常或文件写入异常，以JSON格式返回错误信息
        error_result = {
            "error": "语音合成失败",
            "details": str(e)
        }
        logging.error(f"语音合成失败: {str(e)}")
        print(json.dumps(error_result, ensure_ascii=False))
        # 以非零退出码退出，C++可以捕获到这个错误
        exit(1)

# --- 3. 主函数与命令行解析 ---

def main():
    # 主解析器
    parser = argparse.ArgumentParser(description="AI 服务脚本，通过子命令提供不同功能")
    subparsers = parser.add_subparsers(dest='command', required=True, help='可用的子命令')

    # 子命令: ASR
    parser_understand = subparsers.add_parser('ASR', help='转写音频文件为文本')
    parser_understand.set_defaults(func=handle_audio)

    # 子命令: generate_response
    parser_response = subparsers.add_parser('LLM', help='理解用户请求并规划操作')
    parser_response.add_argument('--user_request', type=str, required=True, help='用户请求文本')
    parser_response.add_argument('--tools_file', type=str, required=True, help='包含Tools定义的JSON文件路径')
    parser_response.add_argument('--previous_turn', type=str, required=False, help='上一轮操作JSON')
    parser_response.set_defaults(func=handle_process_turn)

    # 子命令: tts
    parser_tts = subparsers.add_parser('TTS', help='将文本转换为语音')
    parser_tts.add_argument('--text', type=str, required=True, help='需要转换为语音的文本')
    parser_tts.set_defaults(func=handle_tts)

    # 解析参数并调用对应的处理函数
    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()