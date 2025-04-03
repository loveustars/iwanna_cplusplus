#pragma once
#include <string>
#include <iostream>
#include <sstream> // �����ַ���
#include <vector>
#include <algorithm>
#include <map>     // �洢����
#include "3DPos.h"


class GameHandler {
private:
    PlayerState nowPlayerState;
    PlayerInputState nowInput;

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
        nowPlayerState.pos = { 0.0f, 0.5f, 0.0f};
        nowPlayerState.velocity = { 0.0f, 0.0f, 0.0f };
        nowPlayerState.isInAir = true;
        nowInput = {};
        std::cout << "GameManager initialized. Player start position: "
            << nowPlayerState.pos.toStr() << std::endl;
    }

    // �����������յ�����Ϣ
    void ProcessMessage(const std::string& message) {
        nowInput.jumpPressed = false;
        // �򵥵�Э�����: ���� "INPUT:" ǰ׺
        if (message.rfind("INPUT:", 0) == 0) { // ��INPUT��ͷ����Ϣ
            std::string inputData = message.substr(6); // ��ȡð��֮��Ĳ���
            std::map<std::string, std::string> inputs = TransKeyString(inputData);


            nowInput.moveForward = (inputs.count("W") && inputs["W"] == "1");
            nowInput.moveBackward = (inputs.count("S") && inputs["S"] == "1");
            nowInput.moveLeft= (inputs.count("A") && inputs["A"] == "1");
            nowInput.moveRight = (inputs.count("D") && inputs["D"] == "1");
            if (inputs.count("JUMP") && inputs["JUMP"] == "1") {
                nowInput.jumpPressed = true;
            }

            std::cout << "  New position: " << nowPlayerState.pos.toStr() << std::endl;

        }
        else {
            std::cout << "  Unknown message format." << std::endl;
        }
    }

    void Update(float deltaTime) {
        Struct3D tgVelocity = { 0.0f, nowPlayerState.velocity.y, 0.0f };
        if (nowInput.moveForward) tgVelocity.z += MOVE_SPEED;
        if (nowInput.moveBackward) tgVelocity.z -= MOVE_SPEED;
        if (nowInput.moveLeft) tgVelocity.x -= MOVE_SPEED;
        if (nowInput.moveRight) tgVelocity.x += MOVE_SPEED;
        // (��ѡ) �����Ҫ��ƽ�����ƶ�������ʹ�ò�ֵ����ٶ����ı��ٶȣ�������ֱ������
        // ����Ϊ�˼򵥣�ֱ������Ŀ���ٶ�
        nowPlayerState.velocity.x = tgVelocity.x;
        nowPlayerState.velocity.z = tgVelocity.z;

        // 2. ������Ծ (״̬���߼�: ֻ���ڵ��������)
        if (nowInput.jumpPressed && !nowPlayerState.isInAir) {
            nowPlayerState.velocity.y = JUMP_FORCE; // ʩ�����ϵ���
            nowPlayerState.isInAir = true;      // �뿪����
            std::cout << "Jump!" << std::endl;
        }
        // ������Ծ��״̬��ȷ��ֻ��һ��
        nowInput.jumpPressed = false;


        // 3. Ӧ������ (ֻ�ڿ���ʱ)
        if (nowPlayerState.isInAir) {
            nowPlayerState.velocity.y -= GRAVITY * deltaTime;
            // ������������ٶ�
            nowPlayerState.velocity.y = max(nowPlayerState.velocity.y, MIN_FALL_VELOCITY);
        }


        // 4. ����λ�� (���������ٶȣ�
        nowPlayerState.pos += nowPlayerState.velocity * deltaTime;


        // 5. ��ײ��⣨Ŀǰֻ�е��棩
        //  --- �������������ײ��� ---
        bool prevAir = nowPlayerState.isInAir; // ��¼֮ǰ��״̬

        if (nowPlayerState.pos.y <= GROUND_LEVEL && nowPlayerState.velocity.y <= 0) {
            // ������λ�õ��ڻ���ڵ��棬�������������ֹ
            nowPlayerState.pos.y = GROUND_LEVEL; // ����λ�õ�����
            nowPlayerState.velocity.y = 0;            // ֹͣ��ֱ�ٶ�
            nowPlayerState.isInAir = false;         // ���Ϊ�ڵ�����
            if (!prevAir) {
                std::cout << "Landed." << std::endl;
            }
        }
        else {
            // �������ڿ���
            if (prevAir && nowPlayerState.velocity.y != 0) { // �ӵ������������
                std::cout << "Left ground." << std::endl;
            }
            nowPlayerState.isInAir = true; // ���ڵ���
        }

        // --- ��ʵ��Ϸ��Ҫ�����ӵ���ײ��⣬�漰�ؿ������� ---

    }

    // ��ȡ��ǰ��Ϸ״̬���ַ�����ʾ�����ڷ��͸�ǰ��
    std::string GetStateString() const {
        // ע��Э��: STATE:POS=x,y,z;VEL=vx,vy,vz;AIR=0/1
        std::string stateStr = "STATE:";
        stateStr += "POS=" + nowPlayerState.pos.toStr();
        stateStr += ";V=" + nowPlayerState.velocity.toStr();
        stateStr += ";AIR=" + std::string(nowPlayerState.isInAir ? "1" : "0");
        return stateStr;
    }

};
