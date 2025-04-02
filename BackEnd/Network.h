#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

// 缓冲区大小参数，可修改
const int BUFFER_SIZE = 1024;

class NetworkManager {
private:
    SOCKET listenSocket = INVALID_SOCKET;
    sockaddr_in serverAddr;
    char receiveBuffer[BUFFER_SIZE];
    bool initialized = false;

public:
    // 构造函数：初始化 Winsock
    NetworkManager() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
        }
        else {
            initialized = true;
            std::cout << "Winsock initialized." << std::endl;
        }
    }

    // 析构函数：清理 Winsock 和 Socket
    ~NetworkManager() {
        if (listenSocket != INVALID_SOCKET) {
            closesocket(listenSocket);
            std::cout << "Socket closed." << std::endl;
        }
        if (initialized) {
            WSACleanup();
            std::cout << "Winsock cleaned up." << std::endl;
        }
    }

    // 创建并绑定 Socket
    bool CreateAndBind(short port) {
        if (!initialized) return false;

        listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        std::cout << "UDP Socket created." << std::endl;

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        int result = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
            return false;
        }
        std::cout << "Socket bound to port " << port << "." << std::endl;
        return true;
    }

    // 接收消息，返回接收的字节数，并将消息内容和发送方地址存入参数
    // 返回值: >0 表示成功接收的字节数, 0 通常不可能在UDP阻塞模式下, <0 表示错误
    int ReceiveMessage(std::string& message, sockaddr_in& clientAddr) {
        if (listenSocket == INVALID_SOCKET) return -1;

        memset(receiveBuffer, 0, BUFFER_SIZE); // 清空缓冲区
        int clientAddrSize = sizeof(clientAddr);

        int bytesReceived = recvfrom(listenSocket, receiveBuffer, BUFFER_SIZE - 1, 0, (sockaddr*)&clientAddr, &clientAddrSize);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
            return -1; // 返回错误码
        }

        // 确保字符串是 null-terminated
        receiveBuffer[bytesReceived] = '\0';
        message = receiveBuffer; // 将 C 风格字符串转为 std::string

        // (可选) 打印来源信息
        // char clientIp[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        // int clientPort = ntohs(clientAddr.sin_port);
        // std::cout << "Received " << bytesReceived << " bytes from " << clientIp << ":" << clientPort << ": \"" << message << "\"" << std::endl;

        return bytesReceived;
    }

    // 发送消息到指定的客户端地址
    bool SendMessageTo(const std::string& message, const sockaddr_in& clientAddr) {
        if (listenSocket == INVALID_SOCKET) return false;

        int bytesSent = sendto(listenSocket, message.c_str(), message.length(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));

        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        // std::cout << "Sent " << bytesSent << " bytes: \"" << message << "\"" << std::endl;
        return true;
    }

    // 检查是否初始化成功
    bool IsInitialized() const {
        return initialized && listenSocket != INVALID_SOCKET;
    }
};