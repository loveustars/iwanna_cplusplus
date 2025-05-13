// main.cpp

#include "AsioNetworkManager.h"            // ʹ�û���Asio�����������
#include "GameHandler.h"        // ������Ϸ�߼�������
#include <chrono>               // ����ʱ�����
#include <thread>               // �����߳����� (std::this_thread::sleep_for)��LLM���飩
#include <iostream>
#include <memory>
#include <optional>

// ��������������Ķ˿ں�
constexpr short SERVER_PORT = 12034;
// ����Ŀ����Ϸ�߼�����Ƶ�� (���磬ÿ�� 60 ��)
constexpr float TARGET_UPDATES_PER_SECOND = 60.0f;
// �����ÿ�ι̶����µ�ʱ�䲽�� (delta time)
constexpr float TARGET_DELTA_TIME = 1.0f / TARGET_UPDATES_PER_SECOND;

int main() {
    try {
        // 1.��ʼ��Asio
        // ����Asio�ĺ���I/O�����Ķ��󣬸�����������첽����
        asio::io_context io_context;
        // 2. ��ʼ����Ϸ�߼�������
        GameHandler gameHandler;
        // 3. ��ʼ�����������
        // ʹ��std::make_shared����AsioNetworkManager�Ĺ���ָ�룬���������������첽������(ͨ��weak_ptr/shared_ptr)
        // �� io_context, �˿ں�, �Լ� gameHandler �����ô��ݸ����캯��
        auto networkManager = std::make_shared<AsioNetworkManager>(io_context, SERVER_PORT, gameHandler);
        // �������������Ƿ�ɹ���ʼ�� (�˿��Ƿ�ռ��֮���)
        if (!networkManager->IsInitialized()) {
            std::cerr << "[Main] Error: Network manager failed to initialize. Exiting." << std::endl;
            return 1; // ��ʼ��ʧ�ܣ������˳�
        }

        std::cout << "\n[Main] Backend server started using Asio." << std::endl;
        std::cout << "[Main] Waiting for messages on UDP port " << SERVER_PORT << "..." << std::endl;
        // 4. �����������
        // ����StartReceive()��ʼ�첽�������Կͻ��˵���Ϣ��������������أ�ʵ�ʵĽ��շ�����io_context�ĺ�̨
        networkManager->StartReceive();
        // 5. ��ʼ����ѭ������
        // ���ڴ洢���һ����Чͨ�ŵĿͻ��˶˵���Ϣ
        std::optional<asio::ip::udp::endpoint> last_client_endpoint;
        // ����ѭ���ļ�ʱ����
        auto last_update_time = std::chrono::high_resolution_clock::now(); // �ϴθ��µ�ʱ���
        float accumulator = 0.0f; // ʱ���ۼ������洢���ϴθ�������������ʱ��

        // 6. ��ѭ��
        while (true) { // ѭ��ֱ�������ж�
            // ����֡ʱ��
            auto current_time = std::chrono::high_resolution_clock::now(); // ��ȡ��ǰʱ��
            // �������ϴ�ѭ����������������ʱ��
            std::chrono::duration<float> frame_duration = current_time - last_update_time;
            last_update_time = current_time;                               // �����ϴ�ʱ���
            float frame_time = frame_duration.count();                     // ��ȡ��������ʾ��֡ʱ��
            // ���Ƶ�֡�����ʱ�䣬�����ۼ������˵���һ����ִ�д�������
            if (frame_time > 0.25f) {
                frame_time = 0.25f;
                std::cerr << "[Main] Warning: Frame time > 0.25s, clamping." << std::endl;
            }
            // ����֡������ʱ���ۼӵ��ۼ�����
            accumulator += frame_time;
            // ���������¼�
            // ����io_context.poll() ���������е�ǰ�Ѿ������첽��������¼���GameHandler::ProcessInput���ܻᱻ���ã�����gameHandler����״̬��
            io_context.poll();
            // ��ȡ���ͻ��˵�ַ
            // ��NetworkManager��ȡ���һ�γɹ��յ���Ϣ�Ŀͻ��˵�ַ�������Ժ�����Ϸ״̬���»�ȥ
            last_client_endpoint = networkManager->GetLastClientEndpoint();
            // �̶�ʱ�䲽��������Ϸ�߼�
            // ʹ��whileѭ��ȷ����ʹ֡�ʲ�������Ϸ�߼�Ҳ���Խӽ��㶨�����ʸ���
            while (accumulator >= TARGET_DELTA_TIME) {
                // ����ۼӵ�ʱ���㹻����һ�ι̶������ĸ���
                // ���� GameHandler::Update()������̶���ʱ�䲽�� TARGET_DELTA_TIME
                gameHandler.Update(TARGET_DELTA_TIME);
                // ���ۼ����м�ȥһ��������ʱ��
                accumulator -= TARGET_DELTA_TIME;
            }
            // ѭ��������accumulator ��ʣ�µ��ǲ���һ��������ʱ�䣬���ۼӵ���һ֡
            // ������Ϸ״̬�ؿͻ��ˡ�����Ƿ�֪��Ҫ���ĸ��ͻ��˷���״̬ (�Ƿ��յ�����Ч��Ϣ)
            if (last_client_endpoint) {
                // ���л�
                // ����GameHandler��ȡ��ǰ״̬���л���Ķ���������
                std::optional<std::string> state_data = gameHandler.GetStateDataForNetwork();
                // ������л��Ƿ�ɹ�
                if (state_data) {
                    // ����ɹ�������NetworkManager��SendTo�����첽��������
                    networkManager->SendTo(*state_data, *last_client_endpoint);
                    // ע�⣡������û��������Ƶ�����ƣ����ܻ��Էǳ��ߵ�Ƶ�ʷ���״̬�����������Ҫ������Ҫ���Ʒ������ʡ�
                }
                else {
                    // ���л�ʧ�ܣ���ӡ����
                    std::cerr << "[Main] Warning: Failed to serialize state, not sending update." << std::endl;
                }
            }

            // ��ֹCPUæ��
            // �����ѭ�����е÷ǳ���Ͷ�������һС��ʱ�䣬��CPUʱ���ø��������̣������ת�˷���Դ
            auto loop_end_time = std::chrono::high_resolution_clock::now();            // ��ȡѭ������ʱ��
            std::chrono::duration<float> loop_duration = loop_end_time - current_time; // ���㱾��ѭ����ʱ
            // �򵥵���������������ۼ�����С���ұ���ѭ����ʱҲ�ܶ�
            if (accumulator < TARGET_DELTA_TIME / 2.0f && loop_duration.count() < (TARGET_DELTA_TIME / 2.0f)) {
                // ����1����
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

    }
    // ����catchģ�鲶��˿�ռ�ô���
    catch (const std::runtime_error& e) {
        std::cerr << "[Main] Runtime Error: " << e.what() << std::endl;
        std::cerr << "Please ensure that port " << SERVER_PORT << " is not in use by another application." << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        // �����׼�⼰Asio�����׳����쳣 (�˿ڰ�ʧ��)
        std::cerr << "[Main] Fatal Error: Exception caught: " << e.what() << std::endl;
        return 1; // ���ط����ʾ�����˳�
    }
    catch (...) {
        // ���������������͵�δ֪�쳣
        std::cerr << "[Main] Fatal Error: Unknown exception caught." << std::endl;
        return 1;
    }

    // �����ϲ���ִ�е��������ѭ����ĳ�ַ�ʽ�ж�
    std::cout << "[Main] Server shutting down." << std::endl;
    // �� main ��������ʱ��networkManager(shared_ptr)���Զ����٣�
    // �����������Ḻ��ر�socket��������Դ(Asio�����Զ�����)��
    return 0; // �����˳�
}