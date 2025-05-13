// main.cpp

#include "AsioNetworkManager.h"            // 使用基于Asio的网络管理器
#include "GameHandler.h"        // 包含游戏逻辑处理器
#include <chrono>               // 用于时间计算
#include <thread>               // 用于线程休眠 (std::this_thread::sleep_for)（LLM建议）
#include <iostream>
#include <memory>
#include <optional>

// 定义服务器监听的端口号
constexpr short SERVER_PORT = 12034;
// 定义目标游戏逻辑更新频率 (例如，每秒 60 次)
constexpr float TARGET_UPDATES_PER_SECOND = 60.0f;
// 计算出每次固定更新的时间步长 (delta time)
constexpr float TARGET_DELTA_TIME = 1.0f / TARGET_UPDATES_PER_SECOND;

int main() {
    try {
        // 1.初始化Asio
        // 创建Asio的核心I/O上下文对象，负责调度所有异步操作
        asio::io_context io_context;
        // 2. 初始化游戏逻辑处理器
        GameHandler gameHandler;
        // 3. 初始化网络管理器
        // 使用std::make_shared创建AsioNetworkManager的共享指针，将其生命周期与异步操作绑定(通过weak_ptr/shared_ptr)
        // 将 io_context, 端口号, 以及 gameHandler 的引用传递给构造函数
        auto networkManager = std::make_shared<AsioNetworkManager>(io_context, SERVER_PORT, gameHandler);
        // 检查网络管理器是否成功初始化 (端口是否被占用之类的)
        if (!networkManager->IsInitialized()) {
            std::cerr << "[Main] Error: Network manager failed to initialize. Exiting." << std::endl;
            return 1; // 初始化失败，程序退出
        }

        std::cout << "\n[Main] Backend server started using Asio." << std::endl;
        std::cout << "[Main] Waiting for messages on UDP port " << SERVER_PORT << "..." << std::endl;
        // 4. 启动网络接收
        // 调用StartReceive()开始异步监听来自客户端的消息，这里会立即返回，实际的接收发生在io_context的后台
        networkManager->StartReceive();
        // 5. 初始化主循环变量
        // 用于存储最后一次有效通信的客户端端点信息
        std::optional<asio::ip::udp::endpoint> last_client_endpoint;
        // 更新循环的计时变量
        auto last_update_time = std::chrono::high_resolution_clock::now(); // 上次更新的时间点
        float accumulator = 0.0f; // 时间累加器，存储自上次更新以来经过的时间

        // 6. 主循环
        while (true) { // 循环直到程序被中断
            // 计算帧时间
            auto current_time = std::chrono::high_resolution_clock::now(); // 获取当前时间
            // 计算自上次循环迭代以来经过的时间
            std::chrono::duration<float> frame_duration = current_time - last_update_time;
            last_update_time = current_time;                               // 更新上次时间点
            float frame_time = frame_duration.count();                     // 获取浮点数表示的帧时间
            // 限制单帧的最大时间，避免累加器爆了导致一次性执行大量更新
            if (frame_time > 0.25f) {
                frame_time = 0.25f;
                std::cerr << "[Main] Warning: Frame time > 0.25s, clamping." << std::endl;
            }
            // 将本帧经过的时间累加到累加器中
            accumulator += frame_time;
            // 处理网络事件
            // 调用io_context.poll() 来处理所有当前已就绪的异步操作完成事件。GameHandler::ProcessInput可能会被调用，更新gameHandler输入状态。
            io_context.poll();
            // 获取最后客户端地址
            // 从NetworkManager获取最后一次成功收到消息的客户端地址，用于稍后发送游戏状态更新回去
            last_client_endpoint = networkManager->GetLastClientEndpoint();
            // 固定时间步长更新游戏逻辑
            // 使用while循环确保即使帧率波动，游戏逻辑也能以接近恒定的速率更新
            while (accumulator >= TARGET_DELTA_TIME) {
                // 如果累加的时间足够进行一次固定步长的更新
                // 调用 GameHandler::Update()，传入固定的时间步长 TARGET_DELTA_TIME
                gameHandler.Update(TARGET_DELTA_TIME);
                // 从累加器中减去一个步长的时间
                accumulator -= TARGET_DELTA_TIME;
            }
            // 循环结束后，accumulator 中剩下的是不足一个步长的时间，会累加到下一帧
            // 发送游戏状态回客户端。检查是否知道要向哪个客户端发送状态 (是否收到过有效消息)
            if (last_client_endpoint) {
                // 序列化
                // 调用GameHandler获取当前状态序列化后的二进制数据
                std::optional<std::string> state_data = gameHandler.GetStateDataForNetwork();
                // 检查序列化是否成功
                if (state_data) {
                    // 如果成功，调用NetworkManager的SendTo方法异步发送数据
                    networkManager->SendTo(*state_data, *last_client_endpoint);
                    // 注意！这里我没有做发送频率限制，可能会以非常高的频率发送状态，后面可能需要根据需要限制发送速率。
                }
                else {
                    // 序列化失败，打印警告
                    std::cerr << "[Main] Warning: Failed to serialize state, not sending update." << std::endl;
                }
            }

            // 防止CPU忙等
            // 如果主循环运行得非常快就短暂休眠一小段时间，将CPU时间让给其他进程，避免空转浪费资源
            auto loop_end_time = std::chrono::high_resolution_clock::now();            // 获取循环结束时间
            std::chrono::duration<float> loop_duration = loop_end_time - current_time; // 计算本次循环耗时
            // 简单的休眠条件：如果累加器很小，且本次循环耗时也很短
            if (accumulator < TARGET_DELTA_TIME / 2.0f && loop_duration.count() < (TARGET_DELTA_TIME / 2.0f)) {
                // 休眠1毫秒
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

    }
    // 增加catch模块捕获端口占用错误
    catch (const std::runtime_error& e) {
        std::cerr << "[Main] Runtime Error: " << e.what() << std::endl;
        std::cerr << "Please ensure that port " << SERVER_PORT << " is not in use by another application." << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        // 捕获标准库及Asio可能抛出的异常 (端口绑定失败)
        std::cerr << "[Main] Fatal Error: Exception caught: " << e.what() << std::endl;
        return 1; // 返回非零表示错误退出
    }
    catch (...) {
        // 捕获所有其他类型的未知异常
        std::cerr << "[Main] Fatal Error: Unknown exception caught." << std::endl;
        return 1;
    }

    // 理论上不会执行到这里，除非循环被某种方式中断
    std::cout << "[Main] Server shutting down." << std::endl;
    // 当 main 函数结束时，networkManager(shared_ptr)会自动销毁，
    // 其析构函数会负责关闭socket和清理资源(Asio对象自动管理)。
    return 0; // 正常退出
}