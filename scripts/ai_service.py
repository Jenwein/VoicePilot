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

# --- 2. 任务处理函数 ---

def handle_understand(args):
    """
    处理"理解与规划"任务 (Stage 1)
    接收音频和System Prompt文件路径，返回Function Call JSON
    """
    logging.info(f'开始处理理解任务: 文件路径="{args.file_path}", 工具定义文件="{args.prompt_text}"')

    try:
        # 1. 读取工具定义文件
        logging.info("正在读取工具定义文件...")
        with open(args.prompt_text, 'r', encoding='utf-8') as f:
            function_declarations_list = json.load(f)
        logging.info("工具定义文件读取成功")

        # 2. 上传音频文件
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 29, client.files.upload)
        logging.info("正在上传音频文件...")
        audio_file = client.files.upload(file=args.file_path)
        logging.info(f"音频文件上传成功: {audio_file.name}")

        # 3. 准备 Tools 定义
        #    prompt_text 现在是包含 function_declarations 的JSON文件路径
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 190, GenerateContentConfig)
        tools = types.Tool(function_declarations=function_declarations_list)
        config = types.GenerateContentConfig(tools=[tools])
        # 4. 调用模型发起请求
        #    我们将 "请根据音频内容执行操作" 作为引导模型的指令
        # API 参考: Submodules - Google Gen AI SDK documentation.pdf (Page 51, generate_content)
        instructional_prompt = """
        请仔细听取音频内容，并从可用的工具定义中选择最合适的工具来执行用户的请求。

        如果您能明确理解用户的意图并找到匹配的工具，请使用该工具执行操作。
        
        如果用户的请求不明确或与任何可用工具的功能都不匹配，请返回一个明确的错误信息，说明具体原因，例如：
        - 音频质量问题（如噪音太大、语音不清晰等）
        - 请求内容不完整或不明确
        - 没有找到匹配的工具功能
        
        请不要在不确定的情况下猜测或强行调用工具。
        """
        logging.info("正在向 Gemini API 发送请求...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=[instructional_prompt, audio_file],
            config=config,
        )
        # 5. 从 response 中提取 function call 并格式化为 JSON
        #    参考 FunctionCallingexam.py
        if response.candidates and response.candidates[0].content.parts:
            # 检查是否有 function call
            if hasattr(response.candidates[0].content.parts[0], 'function_call'):
                function_call = response.candidates[0].content.parts[0].function_call
                result = {
                    "functionCall": {
                        "name": function_call.name,
                        "args": dict(function_call.args)
                    }
                }
                print(json.dumps(result, ensure_ascii=False))
            else:
                # 获取模型的文本响应
                error_message = response.candidates[0].content.parts[0].text
                error_response = {
                    "error": "未能识别出明确的函数调用指令",
                    "details": error_message or "无法理解用户请求"
                }
                print(json.dumps(error_response, ensure_ascii=False))
        else:
            print(json.dumps({
                "error": "模型未返回有效响应",
                "details": "服务器返回了空响应"
            }, ensure_ascii=False))

    except FileNotFoundError as e:
        logging.error(f"文件未找到: {str(e)}")
        print(json.dumps({"error": f"文件未找到: {str(e)}"}))
    except json.JSONDecodeError:
        logging.error("Tool 定义文件不是有效的JSON格式。")
        print(json.dumps({"error": "Tool 定义文件不是有效的JSON格式。"}))
    except Exception as e:
        # 捕获其他可能的API调用异常或处理异常
        logging.error(f"处理过程中发生未知错误: {str(e)}")
        print(json.dumps({"error": f"处理过程中发生未知错误: {str(e)}"}))


def handle_generate_response(args):
    """
    处理"响应生成"任务 (Stage 3)
    接收执行结果，返回自然语言回复
    """
    logging.info(f'收到生成响应任务: 执行结果="{args.result_text}"')

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
        logging.info("正在向 Gemini API 发送请求以生成自然语言回复...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=prompt
        )

        # 3. 提取并打印回复（以JSON格式）
        if response.text:
            # 清理一下可能的前后空白
            final_response = response.text.strip()
            # 以JSON格式返回
            result = {
                "response": final_response
            }
            print(json.dumps(result, ensure_ascii=False))
        else:
            # 如果API没有返回文本，提供一个备用回复
            fallback_response = f"操作已完成，结果是：{args.result_text}。"
            result = {
                "response": fallback_response
            }
            print(json.dumps(result, ensure_ascii=False))

    except Exception as e:
        # 捕获API调用异常或处理异常，并返回一个对用户友好的错误信息
        error_response = f"抱歉，我在总结结果时遇到了点麻烦。操作已经执行，其结果是：{args.result_text}。"
        result = {
            "response": error_response,
            "error": str(e)
        }
        logging.error(f"处理过程中发生错误: {e}")
        print(json.dumps(result, ensure_ascii=False))


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
    logging.info(f'收到TTS任务: 文本="{args.text}", 输出到="{args.output_file}"')

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
            write_wave_file(args.output_file, audio_data)
            # 以JSON格式返回成功信息
            result = {
                "status": "success",
                "message": f"语音文件已成功保存到: {args.output_file}"
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
    parser = argparse.ArgumentParser(description="AI 服务脚本，通过子命令提供不同功能。")
    subparsers = parser.add_subparsers(dest='command', required=True, help='可用的子命令')

    # 子命令: understand
    parser_understand = subparsers.add_parser('understand', help='从音频理解用户意图并规划操作。')
    parser_understand.add_argument('--file_path', type=str, required=True, help='输入的音频文件路径。')
    parser_understand.add_argument('--prompt_text', type=str, required=True, help='包含Tools定义的JSON文件路径。')
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