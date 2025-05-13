#include <iostream>
#include <vector>
#include <string>
#include <google/protobuf/stubs/common.h>
#include "messages.pb.h"
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

// 最大允许读取的消息体大小（字节）
constexpr uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024;  // 10 MB

int main() {
    // 初始化 Winsock（只为使用 ntohl）
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[Fatal] WSAStartup failed.\n";
        return -1;
    }

    // 初始化 protobuf 库
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // 设置 Windows 控制台支持 UTF-8（避免中文乱码）
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "[INFO] ServerToClient 消息反序列化测试开始...\n";

    while (true) {
        std::cout << "\n[INFO] 等待输入...\n";

        // Step 1: 读取消息体长度（4 字节，网络序）
        uint32_t net_len = 0;
        if (!std::cin.read(reinterpret_cast<char*>(&net_len), sizeof(net_len))) {
            std::cerr << "[Warning] 无法读取长度，输入流异常或结束，继续...\n";
            std::cin.clear();
            continue;
        }

        uint32_t len = ntohl(net_len);
        if (len == 0 || len > MAX_MESSAGE_SIZE) {
            std::cerr << "[Error] 长度非法: " << len << " 字节，跳过...\n";
            continue;
        }

        // Step 2: 读取消息体
        std::vector<char> buf(len);
        if (!std::cin.read(buf.data(), len)) {
            std::cerr << "[Warning] 读取消息体失败，尝试继续...\n";
            std::cin.clear();
            continue;
        }

        // Step 3: 反序列化
        game_backend::ServerToClient message;
        if (!message.ParseFromArray(buf.data(), static_cast<int>(len))) {
            std::cerr << "[Error] ParseFromArray 失败，数据可能损坏\n";
            continue;
        }

        // Step 4: 打印 DebugString
        std::string debug_text = message.DebugString();
        std::cout << "[Success] DebugString():\n" << debug_text;
        std::cout << "[INFO] 原文长度: " << debug_text.size() << " 字节\n";
    }

    google::protobuf::ShutdownProtobufLibrary();
    WSACleanup();
    return 0;
}
