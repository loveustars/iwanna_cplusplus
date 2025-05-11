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
    // 还需要一个GameHandler的引用，用于处理接收到的消息。
    AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler)
        : io_context_(io_context),
        socket_(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)), // 创建并绑定socket
        game_handler_(game_handler),
        is_initialized_(true) // 如果构造函数没有抛出异常，则假设初始化成功
    {
        // 检查socket是否成功打开和绑定
        if (!socket_.is_open()) {
            is_initialized_ = false;
            throw std::runtime_error("Failed to open or bind UDP socket on port " + std::to_string(port));
        }
        std::cout << "[Network] Asio UDP Socket 创建并成功绑定到端口 " << port << "." << std::endl;
    }

    // 删除拷贝/移动构造函数和赋值运算符，防止对象所有权或切片问题
    AsioNetworkManager(const AsioNetworkManager&) = delete;
    AsioNetworkManager& operator=(const AsioNetworkManager&) = delete;
    AsioNetworkManager(AsioNetworkManager&&) = delete;
    AsioNetworkManager& operator=(AsioNetworkManager&&) = delete;


    // 启动异步接收循环：开始监听传入的消息
    void StartReceive() {
        // 在异步处理程序中使用 weak_ptr，以防止在管理器被销毁后出现悬空引用
        // weak_from_this() 从 enable_shared_from_this 获取一个 weak_ptr
        auto self = weak_from_this();
        socket_.async_receive_from(
            asio::buffer(receive_buffer_), // 提供用于存储接收数据的缓冲区
            remote_endpoint_,              // 提供用于存储发送方地址的 endpoint 对象
            [this, self](const asio::error_code& error, std::size_t bytes_transferred) {
                // 在处理程序内部，先检查 NetworkManager 对象是否仍然存在
                if (auto shared_self = self.lock()) { //尝试从weak_ptr获取shared_ptr
                    // 如果对象存在，则调用内部的接收处理函数
                    HandleReceive(error, bytes_transferred);
                }
                // 如果self.lock()返回空指针，说明NetworkManager销毁了，不用干其他的了
            });
    }

    // 异步地向指定的客户端端点发送消息
    void SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint) {
        // 也用weak_ptr管理生命周期
        auto self = weak_from_this();
        // 复制消息数据 (使用shared_ptr管理)，以确保其生命周期持续到发送操作完成
        // 注意async_send_to是异步的，函数返回时数据可能还未发送，所以这有必要加一步
        auto message_data = std::make_shared<std::string>(message);
        socket_.async_send_to(
            asio::buffer(*message_data), // 要发送的消息数据缓冲区
            target_endpoint,            // 目标客户端的端点信息
            // 发送操作的完成处理程序
            [this, self, message_data](const asio::error_code& error, std::size_t) {
                // 检查NetworkManager对象是否还存在
                if (auto shared_self = self.lock()) {
                    HandleSend(error); // 调用内部的发送处理函数处理潜在错误
                }
            });
    }
    // 检查网络管理器是否已成功初始化 (看socket是否打开)
    bool IsInitialized() const {
        return is_initialized_ && socket_.is_open();
    }
    // 获取最后一次成功接收到有效消息的客户端端点
    std::optional<asio::ip::udp::endpoint> GetLastClientEndpoint() const {
        // 返回存储的端点信息 (可能是空的 std::optional)
        return last_valid_sender_endpoint_;
    }

private:
    void ProcessReceivedData(const std::string&, const asio::ip::udp::endpoint&);
    // 异步接收操作完成后的处理函数
    void HandleReceive(const asio::error_code& error, std::size_t transdBytes) {
        // 检查是否有错误发生，或者错误是消息大小相关的 (UDP 可能截断数据或者丢包)
        if (!error || error == asio::error::message_size) {
            if (transdBytes > 0) { // 确保确实接收到了数据
                // 从接收缓冲区创建std::string (使用实际接收到的字节数)
                std::string received_message(receive_buffer_.data(), transdBytes);
                /*调用数据处理函数，将原始二进制数据和发送方端点传递给数据处理函数，数据处理函数将负责反序列化和进一步的逻辑分发*/
                ProcessReceivedData(received_message, remote_endpoint_);
            }
            else {
                // 收到 0 字节通常不是有效数据，可以忽略或记录警告
                std::cerr << "[Network] Warning: Received 0 bytes." << std::endl;
            }
            // 接收处理完成后，必须再次调用 StartReceive() 来准备接收下一个消息，形成循环
            StartReceive();
        }
        else if (error == asio::error::operation_aborted) {
            // 操作被取消，通常发生在 socket 关闭时 (例如程序退出)
            std::cout << "[Network] Receive operation cancelled (socket likely closed)." << std::endl;
            // 此时不应再调用 StartReceive()
        }
        else {
            // 发生了其他网络错误
            std::cerr << "[Network] Receive error: " << error.message() << " (Code: " << error.value() << ")" << std::endl;
            // 根据错误类型决定是否是致命错误
            // 对于有些严重错误，可能需要停止服务或采取其他措施
            if (error != asio::error::connection_reset) { // 安静地忽略连接重置错误，优先保障游戏
                // 其他错误可能需要更明确地记录
            }
            // 除非是明确的致命错误或程序正在关闭，否则尝试继续接收
            StartReceive(); // 或者根据错误类型决定是否重启接收
        }
    }

    // 异步发送操作完成后的处理函数
    void HandleSend(const asio::error_code& error) {
        // 检查发送过程中是否发生错误 (忽略操作被取消的错误)
        if (error && error != asio::error::operation_aborted) {
            std::cerr << "[Network] Send error: " << error.message() << std::endl;
        }
    }
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