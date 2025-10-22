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

def handle_process_turn(args):
    """
    统一的对话轮次处理函数 (支持多轮对话和复杂任务)
    根据输入参数决定是首轮对话还是后续轮次
    """
    try:
        # 1. 读取工具定义文件
        logging.info("正在读取工具定义文件...")
        with open(args.tool_definitions_path, 'r', encoding='utf-8') as f:
            function_declarations_list = json.load(f)
        logging.info("工具定义文件读取成功")

        # 2. 准备 Tools 定义
        tools = types.Tool(function_declarations=function_declarations_list)
        config = types.GenerateContentConfig(tools=[tools])

        # 3. 构建对话历史 contents
        contents = []
        
        if args.user_input_audio_path:
            # 首轮对话：包含引导prompt和用户音频
            logging.info(f'首轮对话: 音频文件="{args.user_input_audio_path}"')
            
            # 上传音频文件
            logging.info("正在上传音频文件...")
            audio_file = client.files.upload(file=args.user_input_audio_path)
            logging.info(f"音频文件上传成功: {audio_file.name}")
            
            # 构建首轮contents
            instructional_prompt = """
            你是一个智能语音助手。请仔细听取音频内容，理解用户的请求，并从可用的工具中选择合适的工具来执行操作。

            如果需要执行多个步骤来完成用户的请求，请先执行第一步所需的工具。
            如果用户的请求可以通过单个工具完成，请直接调用该工具。
            如果用户的请求不明确或无法通过现有工具完成，请说明具体原因。
            
            请根据用户的实际需求选择最合适的工具执行操作。
            """
            contents = [instructional_prompt, audio_file]
            
        elif args.tool_results_json:
            # 后续轮次：包含之前的工具执行结果
            logging.info(f'后续轮次: 工具结果="{args.tool_results_json}"')
            
            try:
                tool_results = json.loads(args.tool_results_json)
                logging.info(f"解析到 {len(tool_results)} 个工具执行结果")
                
                # 构建包含工具执行结果的对话历史
                # 这里需要重建完整的对话历史，包括之前的函数调用和结果
                instructional_prompt = """
                你是一个智能语音助手。根据之前的工具执行结果，请决定下一步操作：
                
                1. 如果还需要调用其他工具来完成用户的请求，请继续调用相应的工具
                2. 如果所有必要的工具都已执行完毕，请生成一个总结性的自然语言回复
                3. 如果遇到错误或无法继续，请说明具体情况
                
                请根据当前的执行结果和用户的原始需求做出最合适的决策。
                """
                contents = [instructional_prompt]
                
                # 添加工具执行结果到对话历史
                for result in tool_results:
                    tool_name = result.get("tool_name", "unknown")
                    tool_content = result.get("content", "")
                    
                    # 创建函数响应部分
                    function_response_part = types.Part.from_function_response(
                        name=tool_name,
                        response={"result": tool_content}
                    )
                    contents.append(types.Content(role="user", parts=[function_response_part]))
                    
            except json.JSONDecodeError as e:
                logging.error(f"工具结果JSON解析失败: {str(e)}")
                print(json.dumps({
                    "error": "工具结果JSON格式错误",
                    "details": str(e)
                }, ensure_ascii=False, separators=(',', ':')))
                return
        else:
            logging.error("缺少必要的输入参数")
            print(json.dumps({
                "error": "参数错误",
                "details": "必须提供 user_input_audio_path 或 tool_results_json 之一"
            }, ensure_ascii=False, separators=(',', ':')))
            return

        # 4. 调用 Gemini API
        logging.info("正在向 Gemini API 发送请求...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=contents,
            config=config,
        )

        # 5. 解析模型响应
        if response.candidates and response.candidates[0].content.parts:
            parts = response.candidates[0].content.parts
            
            # 检查是否有函数调用
            function_calls = []
            for part in parts:
                if hasattr(part, 'function_call') and part.function_call:
                    function_calls.append({
                        "name": part.function_call.name,
                        "args": dict(part.function_call.args)
                    })
            
            if function_calls:
                # 情况A：模型需要调用工具
                result = {
                    "tool_calls": function_calls
                }
                logging.info(f"模型请求调用 {len(function_calls)} 个工具")
                print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))
            else:
                # 情况B：模型认为任务完成，返回最终回复
                final_text = ""
                for part in parts:
                    if hasattr(part, 'text') and part.text:
                        final_text += part.text
                
                if final_text.strip():
                    result = {
                        "final_response": final_text.strip()
                    }
                    final_text = final_text.replace('`', '"')  # 将反引号替换为双引号
                    logging.info("模型返回最终回复")
                    # 使用ensure_ascii=False和separators来确保输出格式正确
                    print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))
                else:
                    # 没有文本回复也没有函数调用
                    result = {
                        "error": "模型未返回有效响应",
                        "details": "既没有函数调用也没有文本回复"
                    }
                    logging.error("模型响应异常：无有效内容")
                    print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))
        else:
            result = {
                "error": "模型未返回有效响应",
                "details": "响应为空或格式异常"
            }
            logging.error("模型响应为空")
            print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))

    except FileNotFoundError as e:
        logging.error(f"文件未找到: {str(e)}")
        print(json.dumps({
            "error": "文件未找到",
            "details": str(e)
        }, ensure_ascii=False, separators=(',', ':')))
    except json.JSONDecodeError as e:
        logging.error(f"JSON解析错误: {str(e)}")
        print(json.dumps({
            "error": "JSON格式错误",
            "details": str(e)
        }, ensure_ascii=False, separators=(',', ':')))
    except Exception as e:
        logging.error(f"处理过程中发生未知错误: {str(e)}")
        print(json.dumps({
            "error": "处理失败",
            "details": str(e)
        }, ensure_ascii=False, separators=(',', ':')))


# 保留原有函数以兼容旧版本调用
def handle_understand(args):
    """
    兼容性函数：处理"理解与规划"任务 (Stage 1)
    """
    # 转换为新的process_turn调用
    class ProcessTurnArgs:
        def __init__(self):
            self.tool_definitions_path = args.prompt_text
            self.user_input_audio_path = args.file_path
            self.tool_results_json = None
    
    new_args = ProcessTurnArgs()
    handle_process_turn(new_args)


def handle_generate_response(args):
    """
    兼容性函数：处理"响应生成"任务 (Stage 3)
    """
    logging.info(f'收到生成响应任务: 执行结果="{args.result_text}"')

    try:
        prompt = f"""
        你是一个智能语音助手。刚才你的一个工具执行了一个操作，操作的结果是：
        ---
        {args.result_text}
        ---
        请根据这个结果，生成一句简短、友好、口语化的中文回复，告知用户操作的结果。请直接给出最终的回复，不要包含任何额外的解释或前缀。
        """

        logging.info("正在向 Gemini API 发送请求以生成自然语言回复...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=prompt
        )

        if response.text:
            final_response = response.text.strip()
            result = {
                "response": final_response
            }
            print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))
        else:
            fallback_response = f"操作已完成，结果是：{args.result_text}。"
            result = {
                "response": fallback_response
            }
            print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))

    except Exception as e:
        error_response = f"抱歉，我在总结结果时遇到了点麻烦。操作已经执行，其结果是：{args.result_text}。"
        result = {
            "response": error_response,
            "error": str(e)
        }
        logging.error(f"处理过程中发生错误: {e}")
        print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))


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
    try:
        logging.info(f'收到TTS任务: 文本="{args.text}", 输出到="{args.output_file}"')
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
                "message": f"Audio file successfully saved to: {args.output_file}"
            }
            print(json.dumps(result, ensure_ascii=False, separators=(',', ':')))
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
        print(json.dumps(error_result, ensure_ascii=False, separators=(',', ':')))
        logging.error(f"记录日志时出错: {e}")
        # 以非零退出码退出，C++可以捕获到这个错误
        exit(1)

# --- 3. 主函数与命令行解析 ---

def main():
    # 主解析器
    parser = argparse.ArgumentParser(description="AI 服务脚本，通过子命令提供不同功能。")
    subparsers = parser.add_subparsers(dest='command', required=True, help='可用的子命令')

    # 新的统一子命令: process_turn
    parser_process_turn = subparsers.add_parser('process_turn', help='处理一轮对话，支持多轮对话和复杂任务。')
    parser_process_turn.add_argument('--tool_definitions_path', type=str, required=True, help='包含Tools定义的JSON文件路径。')
    parser_process_turn.add_argument('--user_input_audio_path', type=str, required=False, help='用户输入的音频文件路径（仅首轮对话需要）。')
    parser_process_turn.add_argument('--tool_results_json', type=str, required=False, help='上一轮工具执行结果的JSON字符串（后续轮次需要）。')
    parser_process_turn.set_defaults(func=handle_process_turn)

    # 保留旧的子命令以兼容现有C++代码
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