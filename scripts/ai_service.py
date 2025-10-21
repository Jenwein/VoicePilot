import os
import argparse
import json

# 导入 Google AI 和 Google Cloud 的库
from google import genai

# --- 1. 配置 ---
try:
    # 配置 Gemini API Key
    genai.configure(api_key=os.environ["GEMINI_API_KEY"])
except KeyError:
    print("错误: 请先设置 GEMINI_API_KEY 环境变量。")
    exit(1)

# --- 2. 任务处理函数 ---

def handle_understand(args):
    """
    处理“理解与规划”任务 (Stage 1)
    接收音频和System Prompt，返回Function Call JSON
    """
    print(f'-- Python 收到理解任务: 文件路径="{args.file_path}", Prompt长度={len(args.prompt_text)}')
    
    try:
        # 1. 上传音频文件
        # 注意: Gemini API 目前对音频时长和格式有一定要求，请参考官方文档
        print("-- 正在上传音频文件...")
        audio_file = genai.upload_file(path=args.file_path)
        print(f"-- 音频文件上传成功: {audio_file.display_name}")

        # 2. 准备你的 Tools 定义
        # 这是 Function Calling 的核心。你需要在这里定义所有 C++ 端能执行的工具。
        # 这里我们定义一个示例工具 'get_current_time'
        my_tools = [
            {
                "name": "get_current_time",
                "description": "获取当前本地时间。",
                "parameters": {} # 这个函数没有参数
            },
            {
                "name": "open_application",
                "description": "根据指定的名称打开一个应用程序。",
                "parameters": {
                    "type": "OBJECT",
                    "properties": {
                        "app_name": {
                            "type": "STRING",
                            "description": "要打开的应用程序的名称，例如 'notepad.exe' 或 'calculator'。"
                        }
                    },
                    "required": ["app_name"]
                }
            }
            # 在这里可以继续添加更多工具的定义...
        ]

        # 3. 初始化模型
        # 我们使用支持多模态和工具调用的 gemini-1.5-pro-latest 模型
        model = genai.GenerativeModel(
            model_name='gemini-1.5-pro-latest',
            tools=my_tools,
            system_instruction=args.prompt_text  # System Prompt 从命令行传入
        )
        
        # 为了安全，屏蔽一些可能不相关的内容
        safety_settings = {
            HarmCategory.HARM_CATEGORY_HARASSMENT: HarmBlockThreshold.BLOCK_NONE,
            HarmCategory.HARM_CATEGORY_HATE_SPEECH: HarmBlockThreshold.BLOCK_NONE,
            HarmCategory.HARM_CATEGORY_SEXUALLY_EXPLICIT: HarmBlockThreshold.BLOCK_NONE,
            HarmCategory.HARM_CATEGORY_DANGEROUS_CONTENT: HarmBlockThreshold.BLOCK_NONE,
        }

        # 4. 发起请求
        # 将用户的语音（作为音频文件）和 System Prompt 一同发送给模型
        print("-- 正在向 Gemini API 发送请求...")
        response = model.generate_content(
            [audio_file],
            safety_settings=safety_settings,
        )

        # 5. 从 response 中提取 function call 并格式化为 JSON
        if response.candidates and response.candidates[0].content.parts:
            part = response.candidates[0].content.parts[0]
            if part.function_call:
                function_call = part.function_call
                
                # 将 `google.protobuf.struct_pb2.Struct` 转换为 Python 字典
                args_dict = {key: value for key, value in function_call.args.items()}
                
                # 构建最终的 JSON 结构
                output_json = {
                    "functionCall": {
                        "name": function_call.name,
                        "args": args_dict
                    }
                }
                print(json.dumps(output_json, ensure_ascii=False))
            else:
                 print('{"error": "No function call found in response."}')
        else:
            print('{"error": "Invalid response structure."}')
            
    except Exception as e:
        print(f'{{"error": "An error occurred: {str(e)}"}}')


def handle_generate_response(args):
    """
    处理“响应生成”任务 (Stage 3)
    接收执行结果，返回自然语言回复
    """
    print(f'-- Python 收到生成响应任务: 执行结果="{args.result_text}"')
    
    try:
        # 1. 初始化模型
        model = genai.GenerativeModel('gemini-1.5-pro-latest')
        
        # 2. 构建 Prompt
        # 指导模型根据工具的执行结果，生成一句自然、友好、简短的中文回复。
        prompt = f"""
        你是一个AI语音助手。一个工具刚刚在用户的电脑上被执行了，这是它的执行结果字符串: "{args.result_text}"
        请根据这个结果，生成一句自然、友好、简短的中文回复给用户。
        - 如果结果表示成功，就说操作成功了。
        - 如果结果是具体信息（比如时间），就直接告知信息。
        - 如果结果是错误信息，就安抚用户并告知操作失败。
        你的回答应该直接就是回复本身，不要包含任何额外的前缀或解释。
        """
        
        # 3. 发起请求并获取回复
        print("-- 正在请求生成自然语言回复...")
        response = model.generate_content(prompt)
        
        # 4. 打印回复
        print(response.text.strip())
        
    except Exception as e:
        # 如果出错，返回一个通用的错误回复
        print(f"抱歉，我好像遇到了一点问题。错误: {str(e)}")


def handle_tts(args):
    """
    处理“语音合成”任务 (Stage 4)
    接收文本，保存为音频文件
    """
    print(f'-- Python 收到TTS任务: 文本="{args.text}", 输出到="{args.output_file}"')
    
    try:
        # 1. 实例化一个客户端
        client = texttospeech.TextToSpeechClient()

        # 2. 设置要合成的输入文本
        synthesis_input = texttospeech.SynthesisInput(text=args.text)

        # 3. 构建语音请求，选择语言代码 ("en-US") 和一个想要的语音
        # 更多语音选项请参考: https://cloud.google.com/text-to-speech/docs/voices
        voice = texttospeech.VoiceSelectionParams(
            language_code="cmn-CN",  # 中文-普通话
            name="cmn-CN-Wavenet-B", # 一个听起来不错的中文男声
            ssml_gender=texttospeech.SsmlVoiceGender.MALE,
        )

        # 4. 选择音频文件类型
        audio_config = texttospeech.AudioConfig(
            audio_encoding=texttospeech.AudioEncoding.MP3
        )

        # 5. 执行文本到语音的请求
        print("-- 正在请求语音合成...")
        response = client.synthesize_speech(
            input=synthesis_input, voice=voice, audio_config=audio_config
        )

        # 6. 将响应的音频内容写入输出文件
        with open(args.output_file, "wb") as out:
            out.write(response.audio_content)
            print(f"-- 语音文件已成功保存到: {args.output_file}")
            
    except Exception as e:
        print(f"!! TTS Error: {str(e)}", file=sys.stderr)
        exit(1) # 发生错误时，以非零状态码退出，方便C++端捕获


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