#include "Network.h"
#include "GameHandler.h"
#include <chrono>
#include <thread>

const short SERVER_PORT = 8888; // 定义端口（以后可以改）

int main() {
    // 1. 创建管理器实例
    NetworkManager networkManager;
    GameHandler gameHandler; // GameHandler 构造函数会调用 Initialize

    // 2. 初始化网络（创建和绑定 Socket）
    if (!networkManager.CreateAndBind(SERVER_PORT)) {
        std::cerr << "Failed to initialize network. Exiting." << std::endl;
        return 1; // NetworkManager 的析构函数会自动清理 Winsock
    }

    // 检查网络管理器是否准备好
    if (!networkManager.IsInitialized()) {
        std::cerr << "Network manager is not properly initialized. Exiting." << std::endl;
        return 1;
    }

    std::cout << "\nBackend server started successfully. Waiting for messages on port " << SERVER_PORT << "..." << std::endl;

    // 存储上一个客户端的地址信息，以便回复
    sockaddr_in lastClientAddr;
    memset(&lastClientAddr, 0, sizeof(lastClientAddr)); // 初始化
    bool clientAddrKnown = false; // 标记是否知道客户端地址

    // 计算delta时间
    auto lastTime = std::chrono::high_resolution_clock::now();


    // 3. 主循环
    while (true) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTimeDuration = currentTime - lastTime;
        float deltaTime = deltaTimeDuration.count();
        lastTime = currentTime;

        // 防止 deltaTime 过大 (例如调试中断后恢复) 或过小 (可能导致除零等问题)
        deltaTime = min(deltaTime, 0.1f); // 限制最大 deltaTime 为 0.1 秒
        if (deltaTime <= 0.0f) {
            // 如果 deltaTime 太小或为0，可以跳过本次迭代或给一个很小的值
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 短暂休眠避免空转
            continue; // 跳过这一帧的处理
        }
        std::string receivedMessage;
        sockaddr_in nowClientAdd; // 存储当前消息的来源地址

        // 接收消息 (阻塞)
        int bytesReceived = networkManager.ReceiveMessage(receivedMessage, nowClientAdd);

        if (bytesReceived > 0) {
            // 成功接收到消息
            std::cout << "\n--- New Message Received ---" << std::endl;
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &nowClientAdd.sin_addr, clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(nowClientAdd.sin_port);
            std::cout << "From: " << clientIp << ":" << clientPort << std::endl;
            std::cout << "Raw Data: \"" << receivedMessage << "\"" << std::endl;

            // 记录这个客户端地址，以便回复
            lastClientAddr = nowClientAdd;

            // 将消息交给 GameHandler 处理
            gameHandler.ProcessMessage(receivedMessage);

            // 获取最新的游戏状态字符串
            std::string stateString = gameHandler.GetStateString();

            // 将状态发送回发送消息的客户端
            std::cout << "Sending state back: \"" << stateString << "\"" << std::endl;
            if (!networkManager.SendMessageTo(stateString, lastClientAddr)) {
                std::cerr << "Error sending state update." << std::endl;
            }
            std::cout << "--- Message Processed ---" << std::endl;

        }
        else if (bytesReceived < 0) {
            // 发生错误
            std::cerr << "Receive error occurred. Continuing..." << std::endl;
            // 这里可以根据具体的错误码决定是否退出循环，但通常 UDP 错误可以容忍
            // 例如，ICMP Port Unreachable 错误 (WSAECONNRESET) 在 UDP 中可能发生
            int error = WSAGetLastError();
            if (error == WSAECONNRESET) {
                std::cerr << "Warning: Received ICMP Port Unreachable (WSAECONNRESET). Client might have closed." << std::endl;
            }
            else {
                // 其他错误，可能更严重
                std::cerr << "Network receive error: " << error << std::endl;
                // break; // 或者考虑退出？
            }
        }
        gameHandler.Update(deltaTime); // <<<=== 在这里使用 deltaTime 更新游戏状态

        // --- 发送状态回客户端 ---
        if (clientAddrKnown) { // 只有在知道客户端地址时才发送
            std::string stateString = gameHandler.GetStateString();
            // std::cout << "Sending state: " << stateString << std::endl; // 减少打印
            if (!networkManager.SendMessageTo(stateString, lastClientAddr)) {
                std::cerr << "Error sending state update." << std::endl;
                // 如果发送失败多次，可能也意味着客户端关闭了
                // clientAddrKnown = false; // 可以考虑重置
            }
        }


        // bytesReceived == 0 在阻塞 UDP socket 上不应该发生
    }

    // 程序正常结束前，析构函数会自动清理 NetworkManager
    std::cout << "Server shutting down." << std::endl;
    return 0;
}