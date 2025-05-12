// AsioNetwork.h
#pragma once

// 在包含asio头文件之前定义ASIO_STANDALONE，使用独立版本
#define ASIO_STANDALONE
#include <asio.hpp> // Asio头文件
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <array>
#include <memory>
#include <deque>      // 用于消息队列
#include <optional>   // 用std::optional

// 为UDP的包定义一个合理的缓冲区大小
const int UDP_BUFFER_SIZE = 1024; // 可以根据需要调整
// 前向声明GameHandler类，否则有的函数不知道有这个类
class GameHandler;

// 使用Asio进行UDP通信的网络管理器
// 继承自 std::enable_shared_from_this 以便在异步操作中安全地管理自身生命周期
class AsioNetworkManager : public std::enable_shared_from_this<AsioNetworkManager> {
public:
    // 处理接收到的消息
    // 参数为接收到的数据字符串和发送方的端点信息
    using MessageHandler = std::function<void(const std::string&, const asio::ip::udp::endpoint&)>;
    // 构造函数: 需要一个io_context和监听的端口号。
    // 本次更新计划增加端口占用异常提示，于是修改了构造函数
    
    // 还需要一个GameHandler的引用，用于处理接收到的消息。
    AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler);

    // 删除拷贝/移动构造函数和赋值运算符，防止对象所有权或切片问题
    AsioNetworkManager(const AsioNetworkManager&) = delete;
    AsioNetworkManager& operator=(const AsioNetworkManager&) = delete;
    AsioNetworkManager(AsioNetworkManager&&) = delete;
    AsioNetworkManager& operator=(AsioNetworkManager&&) = delete;

    // 启动异步接收循环：开始监听传入的消息
    void StartReceive();

    // 异步地向指定的客户端端点发送消息
    void SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint);
    // 检查网络管理器是否已成功初始化 (看socket是否打开)
    bool IsInitialized() const;
    std::optional<asio::ip::udp::endpoint> GetLastClientEndpoint() const;

private:
    void ProcessReceivedData(const std::string&, const asio::ip::udp::endpoint&);

    // 异步接收操作完成后的处理函数
    void HandleReceive(const asio::error_code& error, std::size_t transdBytes);

    // 异步发送操作完成后的处理函数
    void HandleSend(const asio::error_code& error);

    // Asio 的核心 I/O 服务对象 (持有的是 main 函数中创建的 io_context 的引用)
    asio::io_context& io_context_;
    // 用于UDP通信的socket对象
    asio::ip::udp::socket socket_;
    // 用于存储接收消息时客户端的端点信息
    asio::ip::udp::endpoint remote_endpoint_;
    // 用于接收数据的固定大小缓冲区
    std::array<char, UDP_BUFFER_SIZE> receive_buffer_;
    // GameHandler引用，用于调用其处理接收到的数据
    GameHandler& game_handler_;
    // 标记初始化是否成功的布尔值
    bool is_initialized_ = false;
    // 存储最后一次成功接收到有效消息的客户端端点
    std::optional<asio::ip::udp::endpoint> last_valid_sender_endpoint_;
};