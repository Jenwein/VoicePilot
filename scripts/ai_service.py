# ai_service.py template
import os
import argparse
import json
import wave
from google import genai
from google.genai import types

# --- 1. 配置 ---
try:
    client = genai.Client()
except KeyError:
    print("错误: 请先设置 GEMINI_API_KEY 环境变量。")
    exit(1)

# --- 2. 任务处理函数 ---

def handle_understand(args):
    """
    处理“理解与规划”任务 (Stage 1)
    接收音频和System Prompt，返回Function Call JSON
    """
    print(f'-- Python 开始处理理解任务: 文件路径="{args.file_path}"')

    try:
        # 1. 上传音频文件
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 29, client.files.upload)
        print("-- 正在上传音频文件...")
        audio_file = client.files.upload(file=args.file_path)
        print(f"-- 音频文件上传成功: {audio_file.name}")

        # 2. 准备 Tools 定义
        #    prompt_text 传入的是包含 function_declarations 的JSON字符串
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 190, GenerateContentConfig)
        tool_definitions_json = json.loads(args.prompt_text)
        tools = types.Tool(function_declarations=tool_definitions_json.get("function_declarations", []))
        config = types.GenerateContentConfig(tools=[tools])

        # 3. 调用模型发起请求
        #    我们将 "请根据音频内容执行操作" 作为引导模型的指令
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 51, generate_content)
        print("-- 正在向 Gemini API 发送请求...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=["请根据音频内容执行操作", audio_file],
            config=config,
        )

        # 4. 从 response 中提取 function call 并格式化为 JSON
        #    参考 FunctionCallingexam.py
        if response.candidates and response.candidates[0].content.parts and response.candidates[0].content.parts[0].function_call:
            function_call = response.candidates[0].content.parts[0].function_call
            
            # 使用 json.dumps 将 function_call 对象转换为 JSON 字符串
            # function_call.args 是一个 "Struct" 对象，需要先转换为 dict
            result = {
                "functionCall": {
                    "name": function_call.name,
                    "args": dict(function_call.args)
                }
            }
            # 打印最终的 JSON 字符串，供C++程序读取
            print(json.dumps(result, ensure_ascii=False))
        else:
            # 如果模型没有返回 function_call，可能意味着音频内容不明确或与工具无关
            print(json.dumps({"error": "未能识别出明确的函数调用指令。", "details": response.text or "无详细信息"}))

    except FileNotFoundError:
        print(json.dumps({"error": f"音频文件未找到: {args.file_path}"}))
    except json.JSONDecodeError:
        print(json.dumps({"error": "Tool 定义 (prompt_text) 不是有效的JSON格式。"}))
    except Exception as e:
        # 捕获其他可能的API调用异常或处理异常
        print(json.dumps({"error": f"处理过程中发生未知错误: {str(e)}"}))


def handle_generate_response(args):
    """
    处理“响应生成”任务 (Stage 3)
    接收执行结果，返回自然语言回复
    """
    print(f'-- Python 收到生成响应任务: 执行结果="{args.result_text}"')

    try:
        # 1. 构建 Prompt
        #    这个 prompt 指导模型将一个程序执行结果（可能是JSON，也可能是一个简单的字符串）
        #    转换成一句自然流畅的中文口语回复。
        prompt = f"""
        你是一个智能语音助手。刚才你的一个工具执行了一个操作，操作的结果是：
        ---
        {args.result_text}
        ---
        请根据这个结果，生成一句简短、友好、口语化的中文回复，告知用户操作的结果。请直接给出最终的回复，不要包含任何额外的解释或前缀。
        """

        # 2. 调用模型发起请求
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 51, generate_content)
        print("-- 正在向 Gemini API 发送请求以生成自然语言回复...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=prompt
        )

        # 3. 提取并打印回复
        if response.text:
            # 清理一下可能的前后空白
            final_response = response.text.strip()
            print(final_response)
        else:
            # 如果API没有返回文本，提供一个备用回复
            print(f"操作已完成，结果是：{args.result_text}。")

    except Exception as e:
        # 捕获API调用异常或处理异常，并返回一个对用户友好的错误信息
        print(f"抱歉，我在总结结果时遇到了点麻烦。操作已经执行，其结果是：{args.result_text}。")
        # 您也可以将详细错误打印到 stderr 供调试
        import sys
        print(f"Error in handle_generate_response: {e}", file=sys.stderr)

def write_wave_file(filename: str, pcm_data: bytes, channels: int = 1, sample_width: int = 2, rate: int = 24000):
    """将原始PCM数据写入WAV文件"""
    with wave.open(filename, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(rate)
        wf.writeframes(pcm_data)

def handle_tts(args):
    """
    处理“语音合成”任务 (Stage 4)
    接收文本，保存为音频文件
    """
    print(f'-- Python 收到TTS任务: 文本="{args.text}", 输出到="{args.output_file}"')

    try:
        # 1. 调用 TTS 模型
        #    参考 TTSexam.py 和 Submodules - Google Gen AI SDK documentation.pdf
        #    API 参考: Page 189 (GenerateContentConfig), Page 399 (SpeechConfig)
        print("-- 正在向 Gemini TTS API 发送请求...")
        response = client.models.generate_content(
           model="gemini-1.5-flash-preview-tts-001", # 使用最新的TTS模型
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
            write_wave_file(args.output_file, audio_data)
            print(f"-- 语音文件已成功保存到: {args.output_file}")
            # 成功时，可以什么都不输出，或者输出一个成功标记，由C++检查退出码0即可
        else:
            # 如果API没有返回预期的音频数据
            raise ValueError("API响应中未找到有效的音频数据。")

    except Exception as e:
        # 捕获API调用异常或文件写入异常
        import sys
        print(f"错误: 语音合成失败 - {str(e)}", file=sys.stderr)
        # 以非零退出码退出，C++可以捕获到这个错误
        exit(1)

# --- 3. 主函数与命令行解析 ---

def main():
    # 主解析器
    parser = argparse.ArgumentParser(description="AI 服务脚本，通过子命令提供不同功能。")
    subparsers = parser.add_subparsers(dest='command', required=True, help='可用的子命令')

    # 子命令: understand
    parser_understand = subparsers.add_parser('understand', help='从音频理解用户意图并规划操作。')
    parser_understand.add_argument('--file_path', type=str, required=True, help='输入的音频文件路径。')
    parser_understand.add_argument('--prompt_text', type=str, required=True, help='包含Tools定义的System Prompt。')
    parser_understand.set_defaults(func=handle_understand)

    # 子命令: generate_response
    parser_response = subparsers.add_parser('generate_response', help='根据操作结果生成自然语言回复。')
    parser_response.add_argument('--result_text', type=str, required=True, help='C++执行Tool后的结果字符串。')
    parser_response.set_defaults(func=handle_generate_response)

    # 子命令: tts
    parser_tts = subparsers.add_parser('tts', help='将文本转换为语音。')
    parser_tts.add_argument('--text', type=str, required=True, help='需要转换为语音的文本。')
    parser_tts.add_argument('--output_file', type=str, required=True, help='保存输出音频的文件路径。')
    parser_tts.set_defaults(func=handle_tts)

    # 解析参数并调用对应的处理函数
    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()