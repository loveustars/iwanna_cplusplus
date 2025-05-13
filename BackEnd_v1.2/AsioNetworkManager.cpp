#include "AsioNetworkManager.h"
#include "GameHandler.h"
#include "messages.pb.h"

// ���캯��ʵ��
AsioNetworkManager::AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler)
    : io_context_(io_context),
    socket_(io_context), // �ȹ���socket������������
    game_handler_(game_handler),
    is_initialized_(false) // ��ʼΪδ��ʼ��
{
    asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);
    asio::error_code ec;

    socket_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Failed to open UDP socket: " + ec.message());
    }

    // ����SO_REUSEADDRѡ������������������ʱ���õ�ַ
    // ����ڿ����Ͳ��Խ׶ηǳ����ã����Ա���"Address already in use"����
    // ע�⣺�����������У��Ƿ���Ҫ��ѡ��ȡ���ھ�������
    socket_.set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        // �������ѡ��ʧ�ܣ�����ѡ���¼���棬����һ������������
        std::cerr << "[Network] Warning: Failed to set reuse_address option: " << ec.message() << std::endl;
    }


    socket_.bind(endpoint, ec);
    if (ec) {
        // �����ʧ�ܣ��ر�socket���׳��쳣
        socket_.close(); // ȷ��socket���ر�
        if (ec == asio::error::address_in_use) {
            throw std::runtime_error("UDP Port " + std::to_string(port) + " is already in use.");
        }
        else {
            throw std::runtime_error("Failed to bind UDP socket to port " + std::to_string(port) + ": " + ec.message());
        }
    }

    is_initialized_ = true; // ��ʼ���ɹ�
    std::cout << "[Network] Asio UDP Socket created and successfully bound to port " << port << "." << std::endl;
}


// ������յ���ԭʼ���� (�˺����������л������� GameHandler)
// �����������ݽ�������Ϸ�߼��������
void AsioNetworkManager::ProcessReceivedData(const std::string& data, const asio::ip::udp::endpoint& sender_endpoint) {
    std::cout << "[Network] ProcessReceivedData ENTERED. Data length: " << data.length() << ", Received from: " << sender_endpoint << std::endl;
    std::cout << "[Network] Raw data (hex): ";
    for (size_t i = 0; i < data.length(); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(data[i]) & 0xFF) << " ";
    }
    std::cout << std::dec << std::endl; // �ָ�ʮ�������
    std::cout << "[Network] ProcessReceivedData: Exiting safely (after basic cout)." << std::endl; // �����˳���
    game_backend::ClientToServer client_msg; // ���� Protobuf ��Ϣ����
    std::cout << "[Network] Received " << data.length() << " bytes from " << sender_endpoint << std::endl;
    
    
    // ���Դӽ��յ��Ķ��������� (std::string) ���������Ϣ����
    if (!client_msg.ParseFromString(data)) {
        std::cerr << "[Network] Error: Failed to parse message from "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
        return;
    }
    
    // �ɹ������󣬿��԰�ȫ����Ϊ sender_endpoint ��һ���������¼����Դ
    last_valid_sender_endpoint_ = sender_endpoint;

    std::cout << "[Network] Parsed message successfully. DebugString: " << client_msg.DebugString() << std::endl;
    
    // ��� 'oneof' �����о�����������Ϣ����
    if (client_msg.has_input()) {
        const game_backend::PlayerInput& input_payload = client_msg.input();
        std::cout << "[Network] Message has input payload." << std::endl;
        // �� Protobuf ������ṹ ת��Ϊ �����ڲ�ʹ�õ� PlayerInputState �ṹ
        PlayerInputState input_state;
        input_state.moveForward = input_payload.move_forward();
        input_state.moveBackward = input_payload.move_backward();
        input_state.moveLeft = input_payload.move_left();
        input_state.moveRight = input_payload.move_right();
        input_state.jumpPressed = input_payload.jump_pressed();

        // ���ṹ�����������ݴ��ݸ� GameHandler ���д���
        game_handler_.ProcessInput(input_state);

    }
    else if (client_msg.has_event()) {
        const game_backend::GameEvent& event_payload = client_msg.event();
        std::cout << "[Network] Message has event payload: " << event_payload.type() << std::endl;
    } else {
        // �յ���δ֪�Ļ�յ���Ϣ����
        std::cerr << "[Network] Warning: Received unknown or empty message type from client "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
    }
}

void AsioNetworkManager::StartReceive() {
    auto self = weak_from_this(); // ʹ��weak_ptrȷ����ȫ
    socket_.async_receive_from(
        asio::buffer(receive_buffer_),
        remote_endpoint_,
        [this, self](const asio::error_code& error, std::size_t bytes_transferred) {
            if (auto shared_self = self.lock()) { // �������Ƿ���Ȼ����
                HandleReceive(error, bytes_transferred);
            }
        });
}

void AsioNetworkManager::SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint) {
    auto self = weak_from_this();
    auto message_data = std::make_shared<std::string>(message); // ȷ����Ϣ�������첽�������ǰ��Ч

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
        if (transdBytes > 0 && transdBytes <= receive_buffer_.size()) { // ���ӱ߽���
            // ȷ�� transdBytes ������ receive_buffer_ ��ʵ�ʴ�С
            std::string received_message(receive_buffer_.data(), transdBytes);
            std::cout << "[Network] HandleReceive: transdBytes = " << transdBytes << std::endl; // ���Դ�ӡ
            ProcessReceivedData(received_message, remote_endpoint_);
        }
        else {
            std::cerr << "[Network] Warning: Received 0 bytes." << std::endl;
        }
        StartReceive(); // ����������һ����Ϣ
    }
    else if (error == asio::error::operation_aborted) {
        std::cout << "[Network] Receive operation cancelled (socket likely closed)." << std::endl;
    }
    else {
        std::cerr << "[Network] Receive error: " << error.message() << " (Code: " << error.value() << ")" << std::endl;
        // ���ڷ��������󣬳��Լ�������
        // ������� connection_reset �����Ĵ��󣬶���UDP��˵ͨ�����Ժ��Բ�����
        if (error != asio::error::connection_reset) {
            // �����������͵Ĵ��󣬿�����Ҫ�����ӵĴ����߼�
            // ��Ϊ�˼������������Ȼ���Լ�������
        }
        StartReceive();
    }
}

void AsioNetworkManager::HandleSend(const asio::error_code& error) {
    if (error && error != asio::error::operation_aborted) {
        std::cerr << "[Network] Send error: " << error.message() << std::endl;
    }
}
