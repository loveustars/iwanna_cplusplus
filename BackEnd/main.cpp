#include "Network.h"
#include "GameHandler.h"
const short SERVER_PORT = 8888; // ����˿ڣ��Ժ���Ըģ�

int main() {
    // 1. ����������ʵ��
    NetworkManager networkManager;
    GameHandler gameHandler; // GameHandler ���캯������� Initialize

    // 2. ��ʼ�����磨�����Ͱ� Socket��
    if (!networkManager.CreateAndBind(SERVER_PORT)) {
        std::cerr << "Failed to initialize network. Exiting." << std::endl;
        return 1; // NetworkManager �������������Զ����� Winsock
    }

    // �������������Ƿ�׼����
    if (!networkManager.IsInitialized()) {
        std::cerr << "Network manager is not properly initialized. Exiting." << std::endl;
        return 1;
    }

    std::cout << "\nBackend server started successfully. Waiting for messages on port " << SERVER_PORT << "..." << std::endl;

    // �洢��һ���ͻ��˵ĵ�ַ��Ϣ���Ա�ظ�
    sockaddr_in lastClientAddr;
    memset(&lastClientAddr, 0, sizeof(lastClientAddr)); // ��ʼ��

    // 3. ��ѭ��
    while (true) {
        std::string receivedMessage;
        sockaddr_in nowClientAdd; // �洢��ǰ��Ϣ����Դ��ַ

        // ������Ϣ (����)
        int bytesReceived = networkManager.ReceiveMessage(receivedMessage, nowClientAdd);

        if (bytesReceived > 0) {
            // �ɹ����յ���Ϣ
            std::cout << "\n--- New Message Received ---" << std::endl;
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &nowClientAdd.sin_addr, clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(nowClientAdd.sin_port);
            std::cout << "From: " << clientIp << ":" << clientPort << std::endl;
            std::cout << "Raw Data: \"" << receivedMessage << "\"" << std::endl;

            // ��¼����ͻ��˵�ַ���Ա�ظ�
            lastClientAddr = nowClientAdd;

            // ����Ϣ���� GameHandler ����
            gameHandler.ProcessMessage(receivedMessage);

            // ��ȡ���µ���Ϸ״̬�ַ���
            std::string stateString = gameHandler.GetStateString();

            // ��״̬���ͻط�����Ϣ�Ŀͻ���
            std::cout << "Sending state back: \"" << stateString << "\"" << std::endl;
            if (!networkManager.SendMessageTo(stateString, lastClientAddr)) {
                std::cerr << "Error sending state update." << std::endl;
            }
            std::cout << "--- Message Processed ---" << std::endl;

        }
        else if (bytesReceived < 0) {
            // ��������
            std::cerr << "Receive error occurred. Continuing..." << std::endl;
            // ������Ը��ݾ���Ĵ���������Ƿ��˳�ѭ������ͨ�� UDP �����������
            // ���磬ICMP Port Unreachable ���� (WSAECONNRESET) �� UDP �п��ܷ���
            int error = WSAGetLastError();
            if (error == WSAECONNRESET) {
                std::cerr << "Warning: Received ICMP Port Unreachable (WSAECONNRESET). Client might have closed." << std::endl;
            }
            else {
                // �������󣬿��ܸ�����
                std::cerr << "Network receive error: " << error << std::endl;
                // break; // ���߿����˳���
            }
        }
        // bytesReceived == 0 ������ UDP socket �ϲ�Ӧ�÷���
    }

    // ������������ǰ�������������Զ����� NetworkManager
    std::cout << "Server shutting down." << std::endl;
    return 0;
}