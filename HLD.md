概要设计说明书
版本：1.1
日期：2025年4月21日，星期一 
目录：
1.	引言
1.1目的
1.2 范围
1.3 定义与缩写
1.4 参考文献
2.	系统概述
2.1	设计目标
2.2	系统架构
3.	模块设计
3.1	前端模块设计
3.2	后端模块设计
3.3	通信协议设计
4.	接口设计
4.1	前后端接口
4.2	前端内部接口
4.3	后端内部接口
4.4	外部接口
5.	数据设计
5.1	主要数据结构
5.2	数据存储
6.	NFR实现
1.	引言
1.1.	目的
本文档旨在阐述本游戏的设计方案，明确系统的整体架构、核心模块、关键接口和通信机制，以指导后续的详细设计和开发工作，确保最终实现SRS-v1.2中定义的功能与非功能性需求。
1.2.	范围
1.2.1.	包含的方面：
后端系统架构：
描述C++后端程序的整体结构，包含其主要模块及其相互关系。
核心游戏逻辑实现：涵盖在C++后端中实现的核心玩法机制的高层设计，包括玩家状态管理（PlayerState）、基于输入的玩家移动与重力模拟（GameHandler::Update, GameConstants）、基本的AABB碰撞检测以及相应逻辑。
前端的详细设计与实现：
包括场景搭建、场景交互性的设计、模型渲染、动画状态机、UI界面实现、切换游戏语言、Unity端的网络消息处理逻辑、具体的用户输入捕捉方式、用户输入反馈式等。
关卡内容的目标多样化的设计、关卡奖励机制、关卡剧情与引导等。
1.2.2.	不包含的方面
复杂物理引擎的实现。
SRS中明确排除的功能相关设计。
存储方案的设计。
第三方库的内部实现细节。
1.3.	定义与缩写
HLD：概要设计说明书（High-Level Design）；
Asio：一个用于网络和低层I/O的跨平台C++库；
Protobuf：谷歌的一种数据交换格式与库；
AABB：轴对齐包围盒，用于碰撞检测；
1.4.	参考文献
SRS-v1.2.docx；
Asio：https://think-async.com/Asio/ ；
Protobuf：https://protobuf.dev/ ；
2.	系统概述
2.1.	设计目标
高性能：使用C++处理核心游戏逻辑和物理计算，满足NFR01/NFR02的低延迟要求；
模块化：采用前后端分离架构，降低耦合，便于独立开发和测试；
清晰通信：使用良好协议进行前后端数据交换，提高可维护性（参考NFR06, FR.Proto.Define）；
可拓展性：设计应考虑未来可能添加的功能；
精确控制：后端逻辑应支持SRS中FR03要求的精确控制感。
良好的用户体验：确保游戏界面操作流畅、交互友好，满足NFFR中关于易用性的要求；
高效的图形渲染：优化场景搭建、模型渲染和动画状态机，满足NFR01帧率；
稳定的网络消息处理：确保与后端通信的准确性和及时性，满足NFR02端到端延迟要求。
2.2.	系统架构
架构：客户端-服务器 架构，本地运行；
前端职责：
捕捉用户输入；
渲染游戏世界，角色，UI；
将用户输入打包为Protobuf消息，通过UDP发送至后端；
接收后端发送的Protobuf游戏状态消息；
解析状态消息并更新游戏画面。
后端职责：
监听指定UDP端口，（具体位置见main.cpp）；
接收并解析发送自前端的输入信息；
处理游戏核心逻辑，例如跳跃，移动，碰撞检测等；
维护游戏状态；
将当前游戏状态打包为Protobuf消息；
通过UDP将状态发送回前端；
关键组件（BE）：
main.cpp:初始化系统，开始主循环，管理时间步长，协调网络，游戏逻辑更新。
AsioNetworkManager:封装Asio库，处理UDP套接字，异步接收与发送，反序列化消息。
GameHandler:实现核心游戏规则，处理输入，更新状态，执行碰撞检测，提供序列话接口。
3DPos.h:定义基础的数据类型和游戏常量。
关键组件（FE）：
UnityInputHandler:捕捉各类用户输入，将原始输入数据解析为游戏可识别的指令。
UnityRenderer:承担游戏世界、角色以及UI的渲染工作，管理角色和场景物体的动画播放。
UnityNetworkManager:负责Unity端与后服务器的网络通信，接受后端发送的Protobuf序列化游戏状态消息，进行反序列化处理后，将解析出的游戏状态信息传递给游戏逻辑模块用于更新游戏画面。
简单的架构图：//
3.	模块设计
3.1.	前端模块设计
•	3.1.1 模块名称：UnityInputProcessor
职责：负责在 Unity 环境下捕捉玩家通过键盘、鼠标、手柄等多种设备输入的操作指令，将其转化为游戏内可识别的标准化指令格式，并传递给前端的逻辑处理模块。同时，管理不同输入设备的映射配置，确保输入操作的准确响应。
主要接口：InitializeInput()（初始化输入系统，加载设备映射配置）、CaptureKeyboardInput()（捕捉键盘输入并处理）、CaptureMouseInput()（捕捉鼠标输入并处理）、    CaptureControllerInput()（捕捉手柄输入并处理）、GetProcessedInput()（获取处理后的标准化       输入指令）  
依赖关系：Unity 的Input类、自定义的InputMappingConfig（输入映射配置文件或类）
实现文件：UnityInputProcessor.cs
•	3.1.2 模块名称：UnitySceneRenderer
•	职责：利用 Unity 引擎的渲染功能，负责游戏场景、角色及各类物体的渲染工作。从资源加载、场景布局搭建到光照、阴影等视觉效果设置，全方位打造具有沉浸感的游戏画面。同时，根据后端传来的游戏状态信息实时更新场景中物体的显示状态，如位置、动画等。
•	主要接口：LoadSceneResources()（加载场景所需资源，如模型、材质等）、SetSceneLighting()（设置场景光照效果）、RenderScene()（执行场景渲染操作）、UpdateSceneBasedOnState(GameStateData gameState)（根据游戏状态数据更新场景显示）
•	依赖关系：Unity 的渲染相关 API（如MeshRenderer、Light等组件相关接口）、后端传递的GameStateData（游戏状态数据结构）
•	实现文件：UnitySceneRenderer.cs
•	3.1.3 模块名称：UnityAnimatorController
职责：管理游戏中角色及相关物体的动画系统。通过创建和维护动画状态机，实现动画状态之间的平滑过渡与混合。根据游戏逻辑和玩家操作，触发相应的动画播放，并在动画过程中处理动画事件（如攻击判定、技能释放等）。
主要接口：CreateAnimationStateMachine()（创建动画状态机）、SetAnimationBlendParameters()（设置动画混合参数）、TriggerAnimation(AnimationTriggerType triggerType)（根据触发类型触发动画，如攻击、跳跃等）、HandleAnimationEvents()（处理动画过程中的事件）
依赖关系：Unity 的Animator组件相关 API、自定义的AnimationTriggerType（动画触发类型枚举）
实现文件：UnityAnimatorController.cs
•	3.1.4 模块名称：UnityUIManager
职责：负责游戏中所有用户界面（UI）的设计、创建、管理和交互处理。包括主菜单、游戏内 HUD（抬头显示）、暂停菜单、设置界面等。根据游戏状态实时更新 UI 元素的显示内容，如玩家生命值、得分、关卡进度等，并处理 UI 元素的用户交互事件，如按钮点击、滑块滑动等。
主要接口：CreateUIElements()（创建 UI 元素，如按钮、文本框等）、UpdateUIForGameState(GameStateData gameState)（根据游戏状态更新 UI 显示）、HandleUIButtonClick(UIButton button)（处理 UI 按钮点击事件）、HandleUISliderDrag(UISlider slider)（处理 UI 滑块拖动事件）
依赖关系：Unity 的 UGUI 相关 API、后端传递的GameStateData（游戏状态数据结构）、自定义的UIButton和UISlider（UI 按钮和滑块类）
实现文件：UnityUIManager.cs
•	3.1.5 模块名称：UnityNetworkClient
职责：负责前端与后端基于 UDP 协议的网络通信。与后端的AsioNetworkManager进行对接，发送玩家输入指令信息到后端，并接收后端返回的游戏状态信息（经 Protobuf 序列化）。处理网络连接的建立、维护、断开及重连等操作，同时处理网络消息的加密与解密，确保通信安全。
主要接口：ConnectToServer(string ip, int port)（连接到后端服务器，传入 IP 和端口）、SendInputToServer(PlayerInputState input)（将玩家输入状态发送到后端）、ReceiveGameStateFromServer()（接收后端返回的游戏状态信息）、HandleNetworkDisconnect()（处理网络断开情况）
依赖关系：Unity 的网络相关 API、Protobuf 消息定义文件（对应后端的messages.pb.h等，需在前端引入相关库或生成对应的 C# 代码文件）、自定义的PlayerInputState（玩家输入状态数据结构）
实现文件：UnityNetworkClient.cs

3.2.	后端模块设计
3.2.1.	模块名称：AsioNetworkManager
职责：负责与客户端（前端）的UDP网络通信。封装了Asio库的异步操作。处理传入消息的反序列化并将结构化数据传递给GameHandler。管理客户端端点信息。
主要接口：AsioNetworkManager(io_context, port, gameHandler), StartReceive(), SendTo(message, endpoint), IsInitialized(), GetLastClientEndpoint().
依赖关系：asio::io_context, asio::ip::udp::socket, GameHandler, messages.pb.h.
实现文件：AsioNetworkManager.h, AsioNetworkManager.cpp.
3.2.2.	模块名称：GameHandler
职责：实现核心游戏逻辑，维护玩家状态，根据PlayerInputState和时间步长更新玩家位置与速度。执行简单的AABB碰撞检测与响应，提供将当前PlayerState序列化为Protobuf消息ServerToClient的功能。
主要接口：GameHandler(), Initialize(), ProcessInput(const PlayerInputState&),  Update(float deltaTime), GetStateDataForNetwork().
依赖关系：3DPos.h, messages.pb.h.
实现文件：GameHandler.h
3.2.3.	模块名称：数据结构
职责：定义项目中使用的基本数据类型和物理常量，包括3D
向量，玩家状态，玩家输入，包围盒以及重力等。
主要接口：类定义，常量定义，辅助函数。
依赖关系：C++标准库。
实现文件：3DPos.h.
3.2.4.	模块名称：主循环
职责：初始化所有系统组件，运行主事件循环。管理固定时间步长逻辑，确保游戏以稳定速率更新。驱动Asio事件处理，调用GameHandler的更新，获取并发送游戏状态，包含基本的异常处理。
主要接口：main函数
依赖关系：AsioNetWorkManager, GameHandler, asio::io_context, <chrono>, <thread>.
实现文件：main.cpp
3.2.5.	模块名称：Protobuf消息
职责：定义前后端通信的数据格式。由proto文件编译生成。
主要接口：Vector3, PlayerInput, GameState, ClientToServer, ServerToClient.
依赖关系：Protobuf库
实现文件：messages.pb.h, messages.pb.cc.
3.3.	通信协议设计
选择protobuf作为序列化和通信协议格式，具有高效的优点。
3.3.1.	消息定义：
Vector3: float x; float y; float z;
用途：用于表示位置与速度。
PlayerInput: bool move_forward, bool move_backward; bool move_left, bool jump_pressed;
  用途：由前端发送给后端，表示玩家的操作意图。
  GameState: oneof payload {PlayerInput input = 1; }
  用途：封装客户端到服务器的消息，目前只有玩家输入。
  ServerToClient: oneof payload { GameState state = 1; }
  用途：封装服务器到客户端的消息，目前只有游戏状态。
3.3.2.	通信流程
前端捕捉输入，填充 PlayerInput。
前端创建 ClientToServer 消息，将 PlayerInput 设置为其 payload。
前端序列化 ClientToServer 消息为二进制数据。
前端通过UDP将二进制数据发送到后端监听端口 (8888)。
后端 AsioNetworkManager 接收UDP数据。
后端 AsioNetworkManager::ProcessReceivedData 尝试将数据反序列化为 ClientToServer 消息。
如果成功且包含 input，则提取 PlayerInput 数据，转换为 PlayerInputState 结构体。
AsioNetworkManager 调用 GameHandler::ProcessInput 传递 PlayerInputState。
GameHandler::Update 使用 PlayerInputState 更新 PlayerState。
main 循环调用 GameHandler::GetStateDataForNetwork 获取当前状态。
GetStateDataForNetwork 填充 GameState，创建 ServerToClient 消息，设置 GameState 为其 payload。
GetStateDataForNetwork 将 ServerToClient 序列化为二进制数据并返回。
main 循环调用 AsioNetworkManager::SendTo 将二进制数据通过UDP发送回最后已知的客户端端点。
前端接收UDP数据，反序列化为 ServerToClient，提取 GameState，并据此更新画面。
4.	接口设计
4.1.	前后端接口
接口类型：本地UDP通信接口。
地址：localhost:8888.
数据格式：Protobuf序列化的二进制数据。
消息流：
FE->BE:发送game_backend::ClientToServer消息
BE->FE:发送game_backend::ServerToClient消息
4.2.	前端内部接口
InputManager与NetworkMessageHandler 接口：InputManager 将处理后的 PlayerInput 数据传递给 NetworkMessageHandler，接口方法为 SendPlayerInput (PlayerInput input)，NetworkMessageHandler 接收数据后进行网络发送处理。
RendererManager 与 NetworkMessageHandler 接口：NetworkMessageHandler 在接收到后端的游戏状态更新消息后，调用 RendererManager 的 UpdateGameScene (GameState state) 方法，将更新后的游戏状态传递给 RendererManager，RendererManager 根据状态更新场景中角色和物体的显示。
UIManager与NetworkMessageHandler接口：UIManager 通过 RegisterForGameStateUpdates (NetworkMessageHandler handler) 方法向 NetworkMessageHandler 注册，NetworkMessageHandler 在接收到游戏状态更新消息时，通知 UIManager 更新 UI 显示，如玩家生命值、得分等信息。
UIManager 与 InputManager 接口：当玩家在 UI 上进行操作（如点击按钮）时，UIManager 调用 InputManager 的 HandleUIInput (UIInputEvent event) 方法，将 UI 操作转换为游戏输入指令进行处理。
4.3.	后端内部接口
Main函数调用AsioNetworkManager和GameHandler构造函数，进行初始化。
Main函数调用networkManager->StartReceive开始接收，调用io_context.poll处理异步事件。
AsioNetworkManager::HandleReceive()（内部）调用ProcessReceivedData()。
AsioNetworkManager::ProcessReceivedData()反序列化Protobuf ClientToServer，然后调用 gameHandler.ProcessInput(inputState)。
GameHandler::GetStateDataForNetwork()访问nowPlayerState创建并填充Protobuf GameState，然后序列化 ServerToClient。
4.4.	外部接口
Asio(Standalone) Library:实现UDP通信和事件循环。
Protocol Buffers Library:数据序列化/反序列化。
C++标准库：各个库，比如thread和chrono等。
操作系统：UDP的Socket和时间API。
5.	数据设计
5.1.	主要数据结构
5.1.1.	后端：
C++核心结构：
Struct3D 点或向量。
PlayerState 玩家核心物理状态。
PlayerInputState 一帧以内的输入意图。
AABB Struct3D min, Struct3D max。
游戏常量：重力，速度等。
后端内部状态：
GameHandler::obstacles_: std::vector<AABB> 存储关卡障碍物。
AsioNetworkManager::last_valid_sender_endpoint_: std::optional<asio::ip::udp::endpoint> 存储客户端地址。
通信数据结构：
game_backend::Vector3: 对应Struct3D。
game_backend::PlayerInput: 对应PlayerInputState。
game_backend::GameState: 对应PlayerState的部分字段。
game_backend::ClientToServer: 封装PlayerInput。
game_backend::ServerToClient: 封装GameState。
5.1.2.	前端：
/*待添加*/
5.2.	数据存储
游戏状态只存在于内存中，关闭后即丢失。
关卡地图目前加载于GameHandler构造函数中，未来将转为从文件加载。
6.	NFR实现
6.1.	NFR01-帧率/NFR02-端到端延迟:
使用C++的后端执行计算任务，提高性能。Asio异步网络减少I/O阻塞的可能。UDP作为协议，开销小于其他方法的延迟。固定时间步长目的为确保更稳定的物理模拟基础。Protobuf使用高效的二进制序列化格式，减少传输所需数据量。
6.2.	NFR03-清晰度:
主要由前端Unity负责，后端设计不直接影响视觉清晰度，但需提供准确的状态数据。
6.3.	NFR04-可靠性:
异常处理: main.cpp有顶层try-catch块捕获严重错误。
Asio错误处理: HandleReceive和HandleSend中检查asio::error_code，记录错误信息。
内存管理: 使用std::shared_ptr和std::weak_ptr (AsioNetworkManager) 管理异步操作中的对象生命周期，防止悬空指针。
6.4.	NFR05-游戏平衡及体验:
当前物理非常基础，需要大量调整才能达到“高难但可玩”。HLD层面主要提供了实现这些逻辑的框架。这里不过多说明。
