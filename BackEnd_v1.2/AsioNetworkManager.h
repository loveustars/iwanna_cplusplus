// AsioNetwork.h
#pragma once

// �ڰ���asioͷ�ļ�֮ǰ����ASIO_STANDALONE��ʹ�ö����汾
#define ASIO_STANDALONE
#include <asio.hpp> // Asioͷ�ļ�
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <array>
#include <memory>
#include <deque>      // ������Ϣ����
#include <optional>   // ��std::optional

// ΪUDP�İ�����һ������Ļ�������С
const int UDP_BUFFER_SIZE = 1024; // ���Ը�����Ҫ����
// ǰ������GameHandler�࣬�����еĺ�����֪���������
class GameHandler;

// ʹ��Asio����UDPͨ�ŵ����������
// �̳��� std::enable_shared_from_this �Ա����첽�����а�ȫ�ع���������������
class AsioNetworkManager : public std::enable_shared_from_this<AsioNetworkManager> {
public:
    // ������յ�����Ϣ
    // ����Ϊ���յ��������ַ����ͷ��ͷ��Ķ˵���Ϣ
    using MessageHandler = std::function<void(const std::string&, const asio::ip::udp::endpoint&)>;
    // ���캯��: ��Ҫһ��io_context�ͼ����Ķ˿ںš�
    // ���θ��¼ƻ����Ӷ˿�ռ���쳣��ʾ�������޸��˹��캯��
    
    // ����Ҫһ��GameHandler�����ã����ڴ�����յ�����Ϣ��
    AsioNetworkManager(asio::io_context& io_context, short port, GameHandler& game_handler);

    // ɾ������/�ƶ����캯���͸�ֵ���������ֹ��������Ȩ����Ƭ����
    AsioNetworkManager(const AsioNetworkManager&) = delete;
    AsioNetworkManager& operator=(const AsioNetworkManager&) = delete;
    AsioNetworkManager(AsioNetworkManager&&) = delete;
    AsioNetworkManager& operator=(AsioNetworkManager&&) = delete;

    // �����첽����ѭ������ʼ�����������Ϣ
    void StartReceive();

    // �첽����ָ���Ŀͻ��˶˵㷢����Ϣ
    void SendTo(const std::string& message, const asio::ip::udp::endpoint& target_endpoint);
    // �������������Ƿ��ѳɹ���ʼ�� (��socket�Ƿ��)
    bool IsInitialized() const;
    std::optional<asio::ip::udp::endpoint> GetLastClientEndpoint() const;

private:
    void ProcessReceivedData(const std::string&, const asio::ip::udp::endpoint&);

    // �첽���ղ�����ɺ�Ĵ�����
    void HandleReceive(const asio::error_code& error, std::size_t transdBytes);

    // �첽���Ͳ�����ɺ�Ĵ�����
    void HandleSend(const asio::error_code& error);

    // Asio �ĺ��� I/O ������� (���е��� main �����д����� io_context ������)
    asio::io_context& io_context_;
    // ����UDPͨ�ŵ�socket����
    asio::ip::udp::socket socket_;
    // ���ڴ洢������Ϣʱ�ͻ��˵Ķ˵���Ϣ
    asio::ip::udp::endpoint remote_endpoint_;
    // ���ڽ������ݵĹ̶���С������
    std::array<char, UDP_BUFFER_SIZE> receive_buffer_;
    // GameHandler���ã����ڵ����䴦����յ�������
    GameHandler& game_handler_;
    // ��ǳ�ʼ���Ƿ�ɹ��Ĳ���ֵ
    bool is_initialized_ = false;
    // �洢���һ�γɹ����յ���Ч��Ϣ�Ŀͻ��˶˵�
    std::optional<asio::ip::udp::endpoint> last_valid_sender_endpoint_;
};