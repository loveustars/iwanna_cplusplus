#include "AsioNetworkManager.h"
#include "GameHandler.h"



void AsioNetworkManager::ProcessReceivedData(const std::string& data, const asio::ip::udp::endpoint& sender_endpoint) {
    // --- ���л�ʹ�õ� (�����л�) ---
    game_backend::ClientToServer client_msg; // ���� Protobuf ��Ϣ����

    // ���Դӽ��յ��Ķ��������� (std::string) ���������Ϣ����
    if (!client_msg.ParseFromString(data)) {
        // ����ʧ�ܣ�˵�������𻵻��ʽ����
        std::cerr << "[Network] Error: Failed to parse message from "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
        return; // ���Ը�ʽ�������Ϣ
    }

    // �ɹ�����
    // ��¼���һ�γɹ�ͨ�ŵĿͻ��˶˵�
    last_valid_sender_endpoint_ = sender_endpoint;

    // ��� 'oneof' �����о�����������Ϣ����
    if (client_msg.has_input()) { // �����Ϣ�����������
        const game_backend::PlayerInput& input_payload = client_msg.input(); // ��ȡ��������

        // �� Protobuf ������ṹ ת��Ϊ �����ڲ�ʹ�õ� PlayerInputState �ṹ
        PlayerInputState input_state;
        input_state.moveForward = input_payload.move_forward();
        input_state.moveBackward = input_payload.move_backward();
        input_state.moveLeft = input_payload.move_left();
        input_state.moveRight = input_payload.move_right();
        input_state.jumpPressed = input_payload.jump_pressed();

        // ���ṹ�����������ݴ��ݸ� GameHandler ���д���
        game_handler_.ProcessInput(input_state);

    } // else if (client_msg.has_other_message_type()) { ... } // ������չ����������Ϣ����
    else {
        // �յ���δ֪�Ļ�յ���Ϣ����
        std::cerr << "[Network] Warning: Received unknown or empty message type from client "
            << sender_endpoint.address().to_string() << ":" << sender_endpoint.port() << std::endl;
    }
} 
