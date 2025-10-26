# 重构后的多轮对话流程图

## 新的Agent循环架构

```mermaid
graph TD
    %% 用户交互
    A[用户按键开始录音] --> B[AgentCore::ToggleRecordingAndProcess]
    B --> C[录音完成，调用ProcessAudio]
    
    %% 多轮对话循环
    C --> D[初始化: turnCount=1, toolResults=空数组]
    D --> E{开始循环}
    
    %% 第一轮：音频输入
    E --> F[构建process_turn命令]
    F --> G{是否首轮?}
    G -->|是| H[添加--user_input_audio_path参数]
    G -->|否| I[添加--tool_results_json参数]
    
    %% Python处理
    H --> J[执行Python ai_service.py process_turn]
    I --> J
    J --> K[解析Python返回的JSON]
    
    %% 决策分支
    K --> L{响应类型?}
    
    %% 工具调用分支
    L -->|tool_calls| M[清空toolResults数组]
    M --> N[遍历所有工具调用]
    N --> O[执行ToolRegistry::ExecuteTool]
    O --> P[将结果存入toolResults]
    P --> Q{还有工具?}
    Q -->|是| N
    Q -->|否| R[turnCount++, 继续循环]
    R --> G
    
    %% 最终回复分支
    L -->|final_response| S[提取最终回复文本]
    S --> T[调用GenerateAndSpeakResponse]
    T --> U[执行TTS生成音频]
    U --> V[播放音频]
    V --> W[设置状态为Idle]
    W --> X[结束]
    
    %% 错误处理分支
    L -->|error| Y[记录错误信息]
    Y --> Z[播报错误消息]
    Z --> W
    
    %% 安全检查
    R --> AA{turnCount >= 10?}
    AA -->|是| BB[播报超时消息]
    BB --> W
    AA -->|否| G

    %% 样式
    classDef cpp fill:#e1f5fe
    classDef python fill:#f3e5f5
    classDef decision fill:#fff3e0
    classDef data fill:#e8f5e8
    
    class A,B,C,D,F,G,H,I,M,N,O,P,Q,R,T,U,V,W,X,Y,Z,AA,BB cpp
    class J python
    class E,L,Q,AA decision
    class K,S data
```

## 关键改进点

### 1. 统一的循环架构
- **单一入口**: 所有对话轮次都通过 `process_turn` 命令处理
- **状态管理**: 使用 `toolResults` 数组维护对话历史
- **循环控制**: 基于Python响应类型决定是否继续循环

### 2. 多工具支持
- **并行执行**: 单轮可以执行多个工具
- **结果聚合**: 所有工具结果统一存储和传递
- **依赖处理**: 后续轮次可以使用前面工具的结果

### 3. 智能决策
- **工具调用**: 模型决定需要调用哪些工具
- **任务完成**: 模型判断何时生成最终回复
- **错误恢复**: 完善的错误处理和用户反馈

### 4. 安全机制
- **循环限制**: 最多10轮对话防止无限循环
- **异常处理**: 多层次的错误捕获和处理
- **状态恢复**: 错误时自动返回Idle状态

## 示例场景：复杂任务执行

**用户请求**: "获取当前时间，写入桌面文件"

### 第1轮
- **输入**: 用户音频
- **Python返回**: `{"tool_calls": [{"name": "get_current_time", "args": {}}, {"name": "get_known_folder_path", "args": {"folder_name": "desktop"}}]}`
- **C++执行**: 并行执行两个工具
- **结果**: `[{"tool_name": "get_current_time", "content": "2025-10-22 10:30:00"}, {"tool_name": "get_known_folder_path", "content": "C:/Users/User/Desktop"}]`

### 第2轮
- **输入**: 上轮工具结果
- **Python返回**: `{"tool_calls": [{"name": "write_to_file", "args": {"path": "C:/Users/User/Desktop/time.txt", "content": "当前时间：2025-10-22 10:30:00"}}]}`
- **C++执行**: 写入文件
- **结果**: `[{"tool_name": "write_to_file", "content": "Success: 文件已写入"}]`

### 第3轮
- **输入**: 写入文件结果
- **Python返回**: `{"final_response": "好的，我已经获取了当前时间并保存到您的桌面文件中。"}`
- **C++执行**: TTS播报，循环结束

这个新架构完全支持复杂任务的分解执行，实现了真正的Agent式工作流程。