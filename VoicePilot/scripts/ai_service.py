# ai_service.py - 使用Gemini Chat重构版本
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
        format="%(asctime)s - %(levelname)s - %(message)s",
        handlers=[
            logging.FileHandler(log_file, encoding="utf-8"),
        ],
    )

    # 禁用所有其他库的控制台日志输出
    logging.getLogger().handlers = [
        h
        for h in logging.getLogger().handlers
        if not isinstance(h, logging.StreamHandler)
    ]


# 在模块加载时立即设置日志
setup_logging()


# --- 2. 配置 ---
def _get_client():
    """获取Gemini客户端，如果失败抛出异常"""
    try:
        # api_key = "AIzaSyB8hGKs-VOBuUoDRD4DJ4c4ZTWPWKJdt0g"
        return genai.Client()
    except KeyError:
        logging.error("请先设置 GEMINI_API_KEY 环境变量。")
        raise RuntimeError("环境变量未设置：请先设置 GEMINI_API_KEY 环境变量")


# --- 3. Chat Session 管理 ---
class ChatSession:
    """管理单次对话会话的Chat实例"""

    def __init__(self, tools_file: str = None):
        """
        初始化Chat会话

        Args:
            tools_file (str, optional): 工具定义文件路径
        """
        self.client = _get_client()
        self.chat = None
        self.tools_file = tools_file
        self._initialize_chat()
        logging.info("Chat会话已初始化")

    def _initialize_chat(self):
        """初始化Chat实例，配置工具和系统指令"""
        try:
            # 加载工具定义
            tools = None
            if self.tools_file and os.path.exists(self.tools_file):
                with open(self.tools_file, "r", encoding="utf-8") as f:
                    tools_data = json.load(f)

                    # 处理不同格式的工具定义
                    if isinstance(tools_data, list):
                        tools_definitions = tools_data
                    elif isinstance(tools_data, dict) and "tools" in tools_data:
                        tools_definitions = tools_data["tools"]
                    else:
                        tools_definitions = (
                            [tools_data] if isinstance(tools_data, dict) else []
                        )

                    if tools_definitions:
                        function_declarations = []
                        for tool in tools_definitions:
                            function_declarations.append(
                                types.FunctionDeclaration(
                                    name=tool["name"],
                                    description=tool["description"],
                                    parameters=tool["parameters"],
                                )
                            )
                        tools = [
                            types.Tool(function_declarations=function_declarations)
                        ]
                        logging.info(f"加载了 {len(function_declarations)} 个工具定义")

            # 创建Chat实例
            config = types.GenerateContentConfig(
                tools=tools,
                system_instruction="""你是一个高效的智能语音助手。在处理用户请求时，请遵循以下原则：

1. **并行工具调用**：当需要多个独立的信息时，在同一轮中调用所有相关工具。

**示例处理流程**：
用户请求："获取当前时间,写入桌面文件"
- 第1轮：同时调用 get_current_time() 和 get_known_folder_path(folder_name="desktop")
- 第2轮：使用获取的时间和路径，调用 write_to_file(file_path=桌面路径+"/current_time.txt", content=时间信息)

**错误示例**（避免这样做）：
- 第1轮：只调用 get_current_time()
- 第2轮：只调用 get_known_folder_path()  
- 第3轮：调用 write_to_file()

请分析任务的依赖关系，将无依赖的工具调用合并到同一轮中执行，提高效率。""",
                temperature=0.1,
            )

            self.chat = self.client.chats.create(
                model="gemini-2.5-flash", config=config
            )

        except Exception as e:
            logging.error(f"Chat初始化失败: {e}")
            raise RuntimeError(f"Chat初始化失败: {e}")

    def send_message(self, message: str) -> dict:
        """
        发送消息给Chat

        Args:
            message (str): 用户消息或工具结果

        Returns:
            dict: 处理结果
        """
        try:
            logging.info(f"向Chat发送消息: {message[:100]}...")

            response = self.chat.send_message(message)
            logging.info(f"Full API Response: {response!r}")
            # 解析响应
            result = {
                "status": "finished",
                "function_calls": [],
                "response_text": "",
                "reasoning": "",
            }

            # 检查是否有function call
            if (
                response.candidates
                and response.candidates[0].content
                and response.candidates[0].content.parts
            ):

                has_function_calls = False
                response_text_parts = []

                for part in response.candidates[0].content.parts:
                    if hasattr(part, "function_call") and part.function_call:
                        has_function_calls = True
                        result["function_calls"].append(
                            {
                                "name": part.function_call.name,
                                "args": dict(part.function_call.args),
                            }
                        )
                        logging.info(f"检测到工具调用: {part.function_call.name}")
                    elif hasattr(part, "text") and part.text:
                        response_text_parts.append(part.text)

                # 设置状态和结果
                if has_function_calls:
                    result["status"] = "continue"
                    result["reasoning"] = "需要执行工具调用"
                else:
                    # 没有工具调用，收集文本响应
                    if response_text_parts:
                        result["response_text"] = " ".join(response_text_parts).strip()
                    elif response.text:
                        result["response_text"] = response.text.strip()
                    else:
                        result["response_text"] = "我理解了您的请求。"
                    result["reasoning"] = "对话结束，返回最终回复"

            else:
                # 没有有效响应
                result["response_text"] = "抱歉，我无法理解您的请求。"
                result["reasoning"] = "API响应为空或无效"

            logging.info(
                f"Chat响应: status={result['status']}, function_calls={len(result['function_calls'])}"
            )
            return result

        except Exception as e:
            logging.error(f"Chat消息发送失败: {e}")
            return {"error": "Chat消息发送失败", "details": str(e)}

    def send_function_results(self, function_results: list) -> dict:
        """
        发送工具执行结果给Chat

        Args:
            function_results (list): 工具执行结果列表

        Returns:
            dict: Chat响应
        """
        try:
            # 构建工具结果消息
            parts = []
            for result_info in function_results:
                func_name = result_info.get("name")
                result_data = result_info.get("result", "No result")

                if func_name:
                    parts.append(
                        types.Part.from_function_response(
                            name=func_name, response={"result": result_data}
                        )
                    )
                    logging.info(f"添加工具执行结果: {func_name}")

            if not parts:
                logging.warning("没有有效的工具执行结果")
                return {"error": "工具结果为空", "details": "没有有效的工具执行结果"}

            # 发送工具结果
            logging.info("向Chat发送工具执行结果...")
            response = self.chat.send_message(parts)
            logging.info(f"Full API Response from tool results: {response!r}")
            # 解析响应（与send_message相同的逻辑）
            result = {
                "status": "finished",
                "function_calls": [],
                "response_text": "",
                "reasoning": "",
            }

            if response.candidates and response.candidates[0].content.parts:

                has_function_calls = False
                response_text_parts = []

                for part in response.candidates[0].content.parts:
                    if hasattr(part, "function_call") and part.function_call:
                        has_function_calls = True
                        result["function_calls"].append(
                            {
                                "name": part.function_call.name,
                                "args": dict(part.function_call.args),
                            }
                        )
                        logging.info(f"检测到后续工具调用: {part.function_call.name}")
                    elif hasattr(part, "text") and part.text:
                        response_text_parts.append(part.text)

                if has_function_calls:
                    result["status"] = "continue"
                    result["reasoning"] = "需要执行更多工具调用"
                else:
                    if response_text_parts:
                        result["response_text"] = " ".join(response_text_parts).strip()
                    elif response.text:
                        result["response_text"] = response.text.strip()
                    else:
                        result["response_text"] = "任务已完成。"
                    result["reasoning"] = "所有工具执行完成，返回最终结果"

            logging.info(f"工具结果处理完成: status={result['status']}")
            return result

        except Exception as e:
            logging.error(f"工具结果发送失败: {e}")
            return {"error": "工具结果发送失败", "details": str(e)}


# --- 4. 核心功能函数 ---


def transcribe_audio(audio_file_path: str = None) -> dict:
    """
    音频转录函数 (保持不变)

    Args:
        audio_file_path (str, optional): 音频文件路径，默认为 Resources/audios/input.wav

    Returns:
        dict: 包含转录结果的字典
    """
    if audio_file_path is None:
        audio_file_path = os.path.join("Resources", "audios", "input.wav")

    logging.info(f"收到音频转录请求: 文件路径={audio_file_path}")

    try:
        if not os.path.exists(audio_file_path):
            error_msg = f"音频文件未找到: {audio_file_path}"
            logging.error(error_msg)
            return {
                "error": "音频文件未找到",
                "details": f"文件路径: {audio_file_path}",
            }

        with open(audio_file_path, "rb") as f:
            audio_bytes = f.read()

        client = _get_client()
        logging.info("正在调用 Gemini ASR API...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=[
                "Transcribe the speech to plain Simplified Chinese text. Output only the transcribed text, without any explanations, tags, or formatting.",
                types.Part.from_bytes(
                    data=audio_bytes,
                    mime_type="audio/wav",
                ),
            ],
        )

        if response.text:
            transcript = response.text.strip()
            logging.info(f"转录成功: {transcript}")
            return {"status": "success", "transcript": transcript}
        else:
            error_msg = "API未返回转录文本"
            logging.error(error_msg)
            return {"error": "转录失败", "details": error_msg}

    except Exception as e:
        error_msg = f"音频转录失败: {str(e)}"
        logging.error(error_msg)
        return {"error": "音频转录失败", "details": str(e)}


def transcribe_audio_bytes(audio_bytes: bytes) -> dict:
    """
    音频转录函数 (从内存中的字节数据)

    Args:
        audio_bytes (bytes): 音频文件的原始字节数据.

    Returns:
        dict: 包含转录结果或错误的字典.
    """
    if not audio_bytes:
        error_msg = "传入的音频数据为空"
        logging.error(error_msg)
        return {"error": "无效输入", "details": error_msg}

    logging.info(f"收到音频转录请求: 数据大小={len(audio_bytes)} bytes")

    try:
        client = _get_client()
        logging.info("正在调用 Gemini ASR API...")
        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=[
                "Transcribe the speech to plain Simplified Chinese text. Output only the transcribed text, without any explanations, tags, or formatting.",
                # 直接使用传入的参数 audio_bytes
                types.Part.from_bytes(
                    data=audio_bytes,
                    mime_type="audio/wav",
                ),
            ],
        )

        if response.text:
            transcript = response.text.strip()
            logging.info(f"转录成功: {transcript}")
            return {"status": "success", "transcript": transcript}
        else:
            error_msg = "API未返回转录文本"
            logging.error(error_msg)
            return {"error": "转录失败", "details": error_msg}

    except Exception as e:
        error_msg = f"音频转录失败: {str(e)}"
        logging.error(
            error_msg, exc_info=True
        )  # exc_info=True 可以记录更详细的堆栈信息
        return {"error": "音频转录失败", "details": str(e)}


def create_chat_session(tools_file: str) -> str:
    """
    创建新的Chat会话

    Args:
        tools_file (str): 工具定义文件路径

    Returns:
        str: 会话ID (实际返回"success"表示创建成功)
    """
    try:
        # 这里我们使用全局变量来存储chat session
        # 在实际应用中，你可能需要更复杂的会话管理
        global _current_chat_session
        _current_chat_session = ChatSession(tools_file)
        logging.info("Chat会话创建成功")
        return "success"
    except Exception as e:
        logging.error(f"Chat会话创建失败: {e}")
        return f"error: {e}"


def process_user_message(user_message: str) -> dict:
    """
    处理用户消息 (使用Chat)

    Args:
        user_message (str): 用户消息

    Returns:
        dict: 处理结果
    """
    global _current_chat_session

    if not _current_chat_session:
        return {"error": "Chat会话未初始化", "details": "请先调用create_chat_session"}

    try:
        return _current_chat_session.send_message(user_message)
    except Exception as e:
        logging.error(f"用户消息处理失败: {e}")
        return {"error": "用户消息处理失败", "details": str(e)}


def send_tool_results(tool_results_json: str) -> dict:
    """
    发送工具执行结果

    Args:
        tool_results_json (str): 工具执行结果的JSON字符串

    Returns:
        dict: Chat响应
    """
    global _current_chat_session

    if not _current_chat_session:
        return {"error": "Chat会话未初始化", "details": "请先调用create_chat_session"}

    try:
        # 解析工具结果JSON
        tool_results = json.loads(tool_results_json)
        return _current_chat_session.send_function_results(tool_results)
    except json.JSONDecodeError as e:
        logging.error(f"工具结果JSON解析失败: {e}")
        return {"error": "JSON解析失败", "details": str(e)}
    except Exception as e:
        logging.error(f"工具结果发送失败: {e}")
        return {"error": "工具结果发送失败", "details": str(e)}


def destroy_chat_session() -> str:
    """
    销毁Chat会话

    Returns:
        str: "success" 表示成功
    """
    global _current_chat_session
    _current_chat_session = None
    logging.info("Chat会话已销毁")
    return "success"


def _write_wave_file(
    filename: str,
    pcm_data: bytes,
    channels: int = 1,
    sample_width: int = 2,
    rate: int = 24000,
):
    """将原始PCM数据写入WAV文件"""
    with wave.open(filename, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(rate)
        wf.writeframes(pcm_data)


def synthesize_speech(text: str, output_file_path: str = None) -> dict:
    """
    文本转语音函数 (保持不变)

    Args:
        text (str): 需要转换为语音的文本
        output_file_path (str, optional): 输出音频文件路径

    Returns:
        dict: 包含合成结果的字典
    """
    if output_file_path is None:
        output_file_path = os.path.join("Resources", "audios", "output.wav")

    logging.info(f'收到TTS任务: 文本="{text}", 输出到="{output_file_path}"')

    try:
        os.makedirs(os.path.dirname(output_file_path), exist_ok=True)

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
                            voice_name="Kore",
                        )
                    )
                ),
            ),
        )

        if (
            response.candidates
            and response.candidates[0].content.parts
            and response.candidates[0].content.parts[0].inline_data
        ):
            audio_data = response.candidates[0].content.parts[0].inline_data.data
            _write_wave_file(output_file_path, audio_data)
            logging.info(f"TTS成功，文件保存到: {output_file_path}")

            return {
                "status": "success",
                "message": f"语音文件已成功保存到: {output_file_path}",
            }
        else:
            error_msg = "API响应中未找到有效的音频数据。"
            logging.error(error_msg)
            return {"error": "语音合成失败", "details": error_msg}

    except Exception as e:
        error_msg = f"语音合成失败: {str(e)}"
        logging.error(error_msg)
        return {"error": "语音合成失败", "details": str(e)}


# --- 5. 全局变量 ---
_current_chat_session = None
