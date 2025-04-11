// AsioNetwork.h
#pragma once

// �ڰ��� asio ͷ�ļ�֮ǰ���� ASIO_STANDALONE ��ʹ�ö����汾
#define ASIO_STANDALONE
#include <asio.hpp> // Asio ��Ҫͷ�ļ�
#include <iostream>
#include <string>
#include <vector>
#include <functional> // ���� std::function
#include <array>      // ���ڽ��ջ�����
#include <memory>     // ���� std::enable_shared_from_this, std::shared_ptr
#include <deque>      // ������Ϣ���� (��ѡ)
#include <optional>   // ���� std::optional

// Ϊ UDP ������һ������Ļ�������С
const int UDP_BUFFER_SIZE = 1024; // ���Ը�����Ҫ����

// ǰ������ GameHandler ��
class GameHandler;

// ʹ�� Asio ���� UDP ͨ�ŵ����������
// �̳��� std::enable_shared_from_this �Ա����첽�����а�ȫ�ع���������������
class AsioNetworkManager : public std::enable_shared_from_this<AsioNetworkManager> {
public:
    // �ص��������ͱ��������ڴ�����յ�����Ϣ
    // ����Ϊ���յ�������(�ַ���)�ͷ��ͷ��Ķ˵���Ϣ
    using MessageHandler = std::function<void(const std::string&, const asio::ip::udp::endpoint&)>;

    // ���캯��: ��Ҫһ�� io_context (ͨ���ǹ����) �ͼ����Ķ˿ںš�
    // ����Ҫһ�� GameHandler �����ã����ڴ�����յ�����Ϣ��
    AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler)
        : io_context_(io_context),
        socket_(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)), // �������� socket
        game_handler_(game_handler),
        is_initialized_(true) // ������캯��û���׳��쳣����ٶ���ʼ���ɹ�
    {
        // ��� socket �Ƿ�ɹ��򿪺Ͱ� (��Ȼ���캯�������쳣�������Լ�һ����)
        if (!socket_.is_open()) {
            is_initialized_ = false;
            throw std::runtime_error("Failed to open or bind UDP socket on port " + std::to_string(port));
        }
        std::cout << "[Network] Asio UDP Socket �������ɹ��󶨵��˿� " << port << "." << std::endl;
    }

    // ɾ������/�ƶ����캯���͸�ֵ���������ֹ��������Ȩ����Ƭ����
    AsioNetworkManager(const AsioNetworkManager&) = delete;
    AsioNetworkManager& operator=(const AsioNetworkManager&) = delete;
    AsioNetworkManager(AsioNetworkManager&&) = delete;
    AsioNetworkManager& operator=(AsioNetworkManager&&) = delete;


    // �����첽����ѭ������ʼ�����������Ϣ
    void StartReceive() {
        // ���첽���������ʹ�� weak_ptr���Է�ֹ�ڹ����������ٺ������������
        // weak_from_this() �� enable_shared_from_this ��ȡһ�� weak_ptr
        auto self = weak_from_this();
        socket_.async_receive_from(
            asio::buffer(receive_buffer_), // �ṩ���ڴ洢�������ݵĻ�����
            remote_endpoint_,              // �ṩ���ڴ洢���ͷ���ַ�� endpoint ����
            // Lambda ���ʽ��Ϊ��ɴ������ (Completion Handler)
            [this, self](const asio::error_code& error, std::size_t bytes_transferred) {
                // �ڴ�������ڲ����ȼ�� NetworkManager �����Ƿ���Ȼ����
                if (auto shared_self = self.lock()) { //���Դ� weak_ptr ��ȡ shared_ptr
                    // ���������ڣ�������ڲ��Ľ��մ�����
                    HandleReceive(error, bytes_transferred);
                }
                // ��� self.lock() ���ؿ�ָ�룬˵�� NetworkManager �ѱ����٣������κ���
            });
    }

    // �첽����ָ���Ŀͻ��˶˵㷢����Ϣ
    void SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint) {
        // ͬ��ʹ�� weak_ptr ������������
        auto self = weak_from_this();
        // ������Ϣ���� (ʹ�� shared_ptr ����)����ȷ�����������ڳ��������Ͳ������
        // ������Ϊ async_send_to ���첽�ģ���������ʱ���ݿ��ܻ�δ����
        auto message_data = std::make_shared<std::string>(message);

        socket_.async_send_to(
            asio::buffer(*message_data), // Ҫ���͵���Ϣ���ݻ����� (���Ը��Ƶ� string)
            target_endpoint,            // Ŀ��ͻ��˵Ķ˵���Ϣ
            // ���Ͳ�������ɴ������
            [this, self, message_data](const asio::error_code& error, std::size_t /*bytes_transferred*/) {
                // ��� NetworkManager �����Ƿ񻹴���
                if (auto shared_self = self.lock()) {
                    HandleSend(error); // �����ڲ��ķ��ʹ���������Ǳ�ڴ���
                }
            });
    }

    // �������������Ƿ��ѳɹ���ʼ�� (��Ҫ�� socket �Ƿ��ڴ�״̬)
    bool IsInitialized() const {
        // is_initialized_ �ڹ��캯�������ã�ͬʱ��� socket ״̬���ɿ�
        return is_initialized_ && socket_.is_open();
    }

    // ��ȡ���һ�γɹ����յ���Ч��Ϣ�Ŀͻ��˶˵�
    std::optional<asio::ip::udp::endpoint> GetLastClientEndpoint() const {
        // ���ش洢�Ķ˵���Ϣ (�����ǿյ� std::optional)
        return last_valid_sender_endpoint_;
    }


private:
    void ProcessReceivedData(const std::string&, const asio::ip::udp::endpoint&);
    // �첽���ղ�����ɺ�Ĵ�����
    void HandleReceive(const asio::error_code& error, std::size_t bytes_transferred) {
        // ����Ƿ��д����������ߴ�������Ϣ��С��ص� (UDP ���ܽض�����)
        if (!error || error == asio::error::message_size) {
            if (bytes_transferred > 0) { // ȷ��ȷʵ���յ�������
                // �ӽ��ջ��������� std::string (�ؼ�: ʹ��ʵ�ʽ��յ����ֽ��� bytes_transferred)
                std::string received_message(receive_buffer_.data(), bytes_transferred);

                // --- �������ݴ����� ---
                // ��ԭʼ���������ݺͷ��ͷ��˵㴫�ݸ����ݴ�����
                // ���ݴ��������������л��ͽ�һ�����߼��ַ�
                ProcessReceivedData(received_message, remote_endpoint_);

            }
            else {
                // �յ� 0 �ֽ�ͨ��������Ч���ݣ����Ժ��Ի��¼����
                std::cerr << "[Network] Warning: Received 0 bytes." << std::endl;
            }

            // ���մ�����ɺ�**����**�ٴε��� StartReceive() ��׼��������һ����Ϣ���γ�ѭ��
            StartReceive();

        }
        else if (error == asio::error::operation_aborted) {
            // ������ȡ����ͨ�������� socket �ر�ʱ (��������˳�)
            std::cout << "[Network] Receive operation cancelled (socket likely closed)." << std::endl;
            // ��ʱ��Ӧ�ٵ��� StartReceive()
        }
        else {
            // �����������������
            std::cerr << "[Network] Receive error: " << error.message() << " (Code: " << error.value() << ")" << std::endl;
            // ���ݴ������;����Ƿ�����������
            // ����ĳЩ UDP ���� (����: �������� - ��Ȼ UDP �������ӵģ���ĳЩ OS �ᱨ��)�����ܿ��Ժ��Բ�����
            // �����������󣬿�����Ҫֹͣ������ȡ������ʩ
            if (error != asio::error::connection_reset) { // ʾ��: �����غ����������ô���
                // �������������Ҫ������ؼ�¼
                // Log serious error...
            }
            // ��������ȷ�����������������ڹرգ������Լ�������
            StartReceive(); // ���߸��ݴ������;����Ƿ���������
        }
    }

    // ������յ���ԭʼ���� (�˺����������л������� GameHandler)
    // �����������ݽ�������Ϸ�߼��������
   


    // �첽���Ͳ�����ɺ�Ĵ�����
    void HandleSend(const asio::error_code& error) {
        // ��鷢�͹������Ƿ������� (���Բ�����ȡ���Ĵ���)
        if (error && error != asio::error::operation_aborted) {
            std::cerr << "[Network] Send error: " << error.message() << std::endl;
            // �˴�������ӷ���ʧ�ܵĴ����߼� (���磺��¼��־��֪ͨ��Ϸ�߼���)
        }
        // ���Ͳ�����ɺ�ͨ������Ҫ��ʲô���⴦�������ǰ��败����
    }

    // Asio �ĺ��� I/O ������� (���е��� main �����д����� io_context ������)
    asio::io_context& io_context_;
    // ���� UDP ͨ�ŵ� socket ����
    asio::ip::udp::socket socket_;
    // ���ڴ洢������Ϣʱ�Է�(�ͻ���)�Ķ˵���Ϣ
    asio::ip::udp::endpoint remote_endpoint_;
    // ���ڽ������ݵĹ̶���С������
    std::array<char, UDP_BUFFER_SIZE> receive_buffer_;
    // �� GameHandler �����ã����ڵ����䴦����յ�������
    GameHandler& game_handler_;
    // ��ǳ�ʼ���Ƿ�ɹ��Ĳ���ֵ
    bool is_initialized_ = false;
    // �洢���һ�γɹ����յ���Ч��Ϣ�Ŀͻ��˶˵�
    std::optional<asio::ip::udp::endpoint> last_valid_sender_endpoint_;
};