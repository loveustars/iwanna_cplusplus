#include "AsioNetworkManager.h"
#include "GameHandler.h"
#include "messages.pb.h"

// 构造函数实现
AsioNetworkManager::AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler)
    : io_context_(io_context),
    socket_(io_context), // 先构造socket，但不立即打开
    game_handler_(game_handler),
    is_initialized_(false) // 初始为未初始化
{
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    asio::error_code ec;

    socket_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open UDP socket: " + ec.message());
    }

    // 设置SO_REUSEADDR选项，允许快速重启服务器时重用地址
    // 这对于开发和测试阶段非常有用，可以避免"Address already in use"错误
    // 注意：在生产环境中，是否需要此选项取决于具体需求
    socket_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        // 如果设置选项失败，可以选择记录警告，但不一定是致命错误
        std::cerr << "[Network] Warning: Failed to set reuse_address option: " << ec.message() << std::endl;
    }


    socket_.bind(endpoint, ec);
    if (ec) {
        // 如果绑定失败，关闭socket并抛出异常
        socket_.close(); // 确保socket被关闭
        if (ec == asio::error::address_in_use) {
            throw std::runtime_error("UDP Port " + std::to_string(port) + " is already in use.");
        }
        else {
            throw std::runtime_error("Failed to bind UDP socket to port " + std::to_string(port) + ": " + ec.message());
        }
    }

    is_initialized_ = true; // 初始化成功
    std::cout << "[Network] Asio UDP Socket created and successfully bound to port " << port << "." << std::endl;
}


// 处理接收到的原始数据 (此函数将反序列化并调用 GameHandler)
// 将网络层的数据接收与游戏逻辑处理解耦
void AsioNetworkManager::ProcessReceivedData(const std::string& data, const asio::ip::udp::endpoint& sender_endpoint) {
    std::cout << "[Network] ProcessReceivedData ENTERED. Data length: " << data.length() << ", Received from: " << sender_endpoint << std::endl;
    std::cout << "[Network] Raw data (hex): ";
    for (size_t i = 0; i < data.length(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(data[i]) & 0xFF) << " ";
    }
    std::cout << std::dec << std::endl; // 恢复十进制输出
    std::cout << "[Network] ProcessReceivedData: Exiting safely (after basic cout)." << std::endl; // 新增退出点
    game_backend::ClientToServer client_msg; // 创建 Protobuf 消息对象
    std::cout << "[Network] Received " << data.length() << " bytes from " << sender_endpoint << std::endl;
    
    
    // 尝试从接收到的二进制数据 (std::string) 解析填充消息对象
    if (!client_msg.ParseFromString(data)) {
        std::cerr << "[Network] Error: Failed to parse message from "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
        return;
    }
    
    // 成功解析后，可以安全地认为 sender_endpoint 是一个我们想记录的来源
    last_valid_sender_endpoint_ = sender_endpoint;

    std::cout << "[Network] Parsed message successfully. DebugString: " << client_msg.DebugString() << std::endl;
    
    // 检查 'oneof' 负载中具体是哪种消息类型
    if (client_msg.has_input()) {
        const game_backend::PlayerInput& input_payload = client_msg.input();
        std::cout << "[Network] Message has input payload." << std::endl;
        // 将 Protobuf 的输入结构 转换为 我们内部使用的 PlayerInputState 结构
        PlayerInputState input_state;
        input_state.moveForward = input_payload.move_forward();
        input_state.moveBackward = input_payload.move_backward();
        input_state.moveLeft = input_payload.move_left();
        input_state.moveRight = input_payload.move_right();
        input_state.jumpPressed = input_payload.jump_pressed();

        // 将结构化的输入数据传递给 GameHandler 进行处理
        game_handler_.ProcessInput(input_state);

    }
    else if (client_msg.has_event()) {
        const game_backend::GameEvent& event_payload = client_msg.event();
        std::cout << "[Network] Message has event payload: " << event_payload.type() << std::endl;
    } else {
        // 收到了未知的或空的消息类型
        std::cerr << "[Network] Warning: Received unknown or empty message type from client "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
    }
}

void AsioNetworkManager::StartReceive() {
    auto self = weak_from_this(); // 使用weak_ptr确保安全
    socket_.async_receive_from(
        asio::buffer(receive_buffer_),
        remote_endpoint_,
        [this, self](const asio::error_code& error, std::size_t bytes_transferred) {
            if (auto shared_self = self.lock()) { // 检查对象是否仍然存在
                HandleReceive(error, bytes_transferred);
            }
        });
}

void AsioNetworkManager::SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint) {
    auto self = weak_from_this();
    auto message_data = std::make_shared<std::string>(message); // 确保消息数据在异步操作完成前有效

    socket_.async_send_to(
        asio::buffer(*message_data),
        target_endpoint,
        [this, self, message_data](const asio::error_code& error, std::size_t /*bytes_transferred*/) {
            if (auto shared_self = self.lock()) {
                HandleSend(error);
            }
        });
}

bool AsioNetworkManager::IsInitialized() const {
    return is_initialized_ && socket_.is_open();
}

std::optional<asio::ip::udp::endpoint> AsioNetworkManager::GetLastClientEndpoint() const {
    return last_valid_sender_endpoint_;
}

void AsioNetworkManager::HandleReceive(const asio::error_code& error, std::size_t transdBytes) {
    if (!error || error == asio::error::message_size) {
        if (transdBytes > 0 && transdBytes <= receive_buffer_.size()) { // 增加边界检查
            // 确保 transdBytes 不超过 receive_buffer_ 的实际大小
            std::string received_message(receive_buffer_.data(), transdBytes);
            std::cout << "[Network] HandleReceive: transdBytes = " << transdBytes << std::endl; // 调试打印
            ProcessReceivedData(received_message, remote_endpoint_);
        }
        else {
            std::cerr << "[Network] Warning: Received 0 bytes." << std::endl;
        }
        StartReceive(); // 继续接收下一个消息
    }
    else if (error == asio::error::operation_aborted) {
        std::cout << "[Network] Receive operation cancelled (socket likely closed)." << std::endl;
    }
    else {
        std::cerr << "[Network] Receive error: " << error.message() << " (Code: " << error.value() << ")" << std::endl;
        // 对于非致命错误，尝试继续接收
        // 如果是像 connection_reset 这样的错误，对于UDP来说通常可以忽略并继续
        if (error != asio::error::connection_reset) {
            // 对于其他类型的错误，可能需要更复杂的处理逻辑
            // 但为了简单起见，我们仍然尝试继续接收
        }
        StartReceive();
    }
}

void AsioNetworkManager::HandleSend(const asio::error_code& error) {
    if (error && error != asio::error::operation_aborted) {
        std::cerr << "[Network] Send error: " << error.message() << std::endl;
    }
}
