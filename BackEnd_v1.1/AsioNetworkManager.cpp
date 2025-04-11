#include "AsioNetworkManager.h"
#include "GameHandler.h"



void AsioNetworkManager::ProcessReceivedData(const std::string& data, const asio::ip::udp::endpoint& sender_endpoint) {
    // --- 序列化使用点 (反序列化) ---
    game_backend::ClientToServer client_msg; // 创建 Protobuf 消息对象

    // 尝试从接收到的二进制数据 (std::string) 解析填充消息对象
    if (!client_msg.ParseFromString(data)) {
        // 解析失败，说明数据损坏或格式不符
        std::cerr << "[Network] Error: Failed to parse message from "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
        return; // 忽略格式错误的消息
    }

    // 成功解析
    // 记录最后一次成功通信的客户端端点
    last_valid_sender_endpoint_ = sender_endpoint;

    // 检查 'oneof' 负载中具体是哪种消息类型
    if (client_msg.has_input()) { // 如果消息包含玩家输入
        const game_backend::PlayerInput& input_payload = client_msg.input(); // 获取输入数据

        // 将 Protobuf 的输入结构 转换为 我们内部使用的 PlayerInputState 结构
        PlayerInputState input_state;
        input_state.moveForward = input_payload.move_forward();
        input_state.moveBackward = input_payload.move_backward();
        input_state.moveLeft = input_payload.move_left();
        input_state.moveRight = input_payload.move_right();
        input_state.jumpPressed = input_payload.jump_pressed();

        // 将结构化的输入数据传递给 GameHandler 进行处理
        game_handler_.ProcessInput(input_state);

    } // else if (client_msg.has_other_message_type()) { ... } // 可以扩展处理其他消息类型
    else {
        // 收到了未知的或空的消息类型
        std::cerr << "[Network] Warning: Received unknown or empty message type from client "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
    }
} 
