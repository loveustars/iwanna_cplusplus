#pragma once
#include <string>
#include <iostream>
#include <sstream> // �����ַ���
#include <vector>
#include <map>     // �洢����
#include "3DPos.h"

// (ȷ�� Vector3D �� PlayerState �ṹ�Ѷ���)

class GameHandler {
private:
    PlayerState nowPlayerState;

    // ����ǰ�˷��ر�ʾ������ַ���
    std::map<std::string, std::string> TransKeyString(const std::string& s) {
        std::map<std::string, std::string> result;
        std::stringstream ss(s);
        std::string segment;

        while (std::getline(ss, segment, ',')) { // �����ŷָ�
            size_t equalsPos = segment.find('=');
            if (equalsPos != std::string::npos) {
                std::string key = segment.substr(0, equalsPos);
                std::string value = segment.substr(equalsPos + 1);
                result[key] = value;
            }
        }
        return result;
    }


public:
    // ��ʼ����Ϸ״̬�Ĺ��캯��
    GameHandler() {
        Initialize();
    }

    // ��ʼ����Ϸ״̬
    void Initialize() {
        nowPlayerState.pos = { 0.0, 0.5, 0.0};
        std::cout << "GameManager initialized. Player start position: "
            << nowPlayerState.pos.toStr() << std::endl;
    }

    // �����������յ�����Ϣ
    void ProcessMessage(const std::string& message) {
        std::cout << "GameManager processing message: \"" << message << "\"" << std::endl;

        // �򵥵�Э�����: ���� "INPUT:" ǰ׺
        if (message.rfind("INPUT:", 0) == 0) { // ��INPUT��ͷ����Ϣ
            std::string inputData = message.substr(6); // ��ȡð��֮��Ĳ���
            std::map<std::string, std::string> inputs = TransKeyString(inputData);

            // --- ��������ݽ������� inputs ������Ϸ״̬ ---
            // ����һ���ǳ���������ʾ��ʵ����Ϸ��Ҫ�����ӵ������״̬��
            float moveSpeed = 0.1f; // ÿ���ƶ��ľ��루Ӧ�ó��� deltaTime��
            bool jumped = false;

            if (inputs.count("W") && inputs["W"] == "1") {
                nowPlayerState.pos.y += moveSpeed; // ���� Z ����ǰ
                std::cout << "  Input: Move Forward" << std::endl;
            }
            if (inputs.count("S") && inputs["S"] == "1") {
                nowPlayerState.pos.y -= moveSpeed;
                std::cout << "  Input: Move Backward" << std::endl;
            }
            if (inputs.count("A") && inputs["A"] == "1") {
                nowPlayerState.pos.x -= moveSpeed; // ���� X ������
                std::cout << "  Input: Move Left" << std::endl;
            }
            if (inputs.count("D") && inputs["D"] == "1") {
                nowPlayerState.pos.x += moveSpeed;
                std::cout << "  Input: Move Right" << std::endl;
            }
            if (inputs.count("JUMP") && inputs["JUMP"] == "1") {
                // ����ֻ�Ǽ򵥵������ƶ�һ�㣬��������Ծ��Ҫ����ģ��
                // ������Ҫ����Ƿ��ڵ����״̬
                nowPlayerState.pos.z += moveSpeed * 2.0;
                jumped = true;
                std::cout << "  Input: Jump (applied simple Y increase)" << std::endl;
            }

            // �򵥵�ģ�����������û��Ծ�����µ�һ�㣬ֱ�����أ�
            // ����һ���ǳ��ֲڵ�ģ�⣬ʵ����Ҫ��ײ���
            if (!jumped && nowPlayerState.pos.y > 0.0f) {
                nowPlayerState.pos.y -= moveSpeed * 0.5f;
                if (nowPlayerState.pos.y < 0.0f) {
                    nowPlayerState.pos.y = 0.0f; // ��������� Y=0
                }
            }

            std::cout << "  New position: " << nowPlayerState.pos.toStr() << std::endl;

        }
        else {
            std::cout << "  Unknown message format." << std::endl;
        }
    }

    // ��ȡ��ǰ��Ϸ״̬���ַ�����ʾ�����ڷ��͸�ǰ��
    std::string GetStateString() const {
        // Э��: STATE:POS=x,y,z
        return "STATE:POS=" + nowPlayerState.pos.toStr();
    }

    // (δ���������)
    // void Update(float deltaTime) {
    //     // �������ʱ��ĸ��£�������ģ�⡢����״̬��
    // }
};
