/*#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h> // 包含一些更新的地址转换函数等
#include <string>
#include <vector> // 后面可能用来存储消息，好用的容器

// 属性里要设附加依赖项
#pragma comment(lib, "Ws2_32.lib")

// 定义一些常量，方便后面修改
const short SERVER_PORT = 8888; // C++ 后端监听的端口
const int BUFFER_SIZE = 1024;   // 接收缓冲区大小

int main() {
    // 1. 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }
    std::cout << "Winsock initialized." << std::endl;

    SOCKET serverSocket = INVALID_SOCKET;

    // 2. 创建 Socket (UDP)
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "UDP Socket created." << std::endl;

    // 3. 准备服务器地址结构
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_port = htons(SERVER_PORT); // 监听的端口号，htons 转换字节序
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 监听来自任何网络接口的连接

    // 4. 绑定 Socket 到地址和端口
    result = bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Socket bound to port " << SERVER_PORT << "." << std::endl;

    std::cout << "Backend server started. Waiting for messages..." << std::endl;

    char receiveBuffer[BUFFER_SIZE]; // 创建接收缓冲区
    sockaddr_in clientAdd;         // 用于存储发送方（Unity 前端）的地址信息
    int clientAddSize = sizeof(clientAdd);

    // 5. 接收数据循环
    while (true) { // 这是一个简单的无限循环，按 Ctrl+C 停止程序
        memset(receiveBuffer, 0, BUFFER_SIZE); // 清空缓冲区

        // 尝试接收数据 (recvfrom 是阻塞的)
        int byteRcv = recvfrom(serverSocket, receiveBuffer, BUFFER_SIZE, 0, (sockaddr*)&clientAdd, &clientAddSize);

        if (byteRcv == SOCKET_ERROR) {
            std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
            // 在某些错误情况下可能需要退出循环，这里为了简单先继续
            continue; // 或者 break;
        }

        // 将接收到的客户端地址信息转换成字符串，方便查看
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAdd.sin_addr, clientIp, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAdd.sin_port);

        std::cout << "Received " << byteRcv << " bytes from " << clientIp << ":" << clientPort << std::endl;

        // 打印接收到的原始数据 (假设是字符串，后面需要定义协议来解析)
        std::cout << "Message: \"" << receiveBuffer << "\"" << std::endl;

        // --- 接下来在这里处理接收到的消息，并准备响应 ---
        // --- 目前只是打印出来 ---

        // 发送一个简单的确认消息回 Unity (演示)
        std::string response = "Message received by C++ backend!";
        int bytesSent = sendto(serverSocket, response.c_str(), response.length(), 0, (sockaddr*)&clientAdd, clientAddSize);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "Sent confirmation back to client." << std::endl;
        }
    }

    // 关闭 Socket
    closesocket(serverSocket);
    std::cout << "Socket closed." << std::endl;

    // 清理 Winsock
    WSACleanup();
    return 0;
}*/
