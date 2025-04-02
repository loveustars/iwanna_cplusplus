#pragma once
#include <string>
#include <iostream>
#include <sstream> // 解析字符串
#include <vector>
#include <map>     // 存储输入
#include "3DPos.h"

// (确保 Vector3D 和 PlayerState 结构已定义)

class GameHandler {
private:
    PlayerState nowPlayerState;

    // 解析前端返回表示输入的字符串
    std::map<std::string, std::string> TransKeyString(const std::string& s) {
        std::map<std::string, std::string> result;
        std::stringstream ss(s);
        std::string segment;

        while (std::getline(ss, segment, ',')) { // 按逗号分割
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
    // 初始化游戏状态的构造函数
    GameHandler() {
        Initialize();
    }

    // 初始化游戏状态
    void Initialize() {
        nowPlayerState.pos = { 0.0, 0.5, 0.0};
        std::cout << "GameManager initialized. Player start position: "
            << nowPlayerState.pos.toStr() << std::endl;
    }

    // 处理从网络接收到的消息
    void ProcessMessage(const std::string& message) {
        std::cout << "GameManager processing message: \"" << message << "\"" << std::endl;

        // 简单的协议解析: 查找 "INPUT:" 前缀
        if (message.rfind("INPUT:", 0) == 0) { // 找INPUT开头的消息
            std::string inputData = message.substr(6); // 获取冒号之后的部分
            std::map<std::string, std::string> inputs = TransKeyString(inputData);

            // --- 在这里根据解析出的 inputs 更新游戏状态 ---
            // 这是一个非常基础的演示，实际游戏需要更复杂的物理和状态机
            float moveSpeed = 0.1f; // 每次移动的距离（应该乘以 deltaTime）
            bool jumped = false;

            if (inputs.count("W") && inputs["W"] == "1") {
                nowPlayerState.pos.y += moveSpeed; // 假设 Z 是向前
                std::cout << "  Input: Move Forward" << std::endl;
            }
            if (inputs.count("S") && inputs["S"] == "1") {
                nowPlayerState.pos.y -= moveSpeed;
                std::cout << "  Input: Move Backward" << std::endl;
            }
            if (inputs.count("A") && inputs["A"] == "1") {
                nowPlayerState.pos.x -= moveSpeed; // 假设 X 是向左
                std::cout << "  Input: Move Left" << std::endl;
            }
            if (inputs.count("D") && inputs["D"] == "1") {
                nowPlayerState.pos.x += moveSpeed;
                std::cout << "  Input: Move Right" << std::endl;
            }
            if (inputs.count("JUMP") && inputs["JUMP"] == "1") {
                // 这里只是简单地向上移动一点，真正的跳跃需要物理模拟
                // 并且需要检查是否在地面等状态
                nowPlayerState.pos.z += moveSpeed * 2.0;
                jumped = true;
                std::cout << "  Input: Jump (applied simple Y increase)" << std::endl;
            }

            // 简单的模拟重力（如果没跳跃就往下掉一点，直到触地）
            // 这是一个非常粗糙的模拟，实际需要碰撞检测
            if (!jumped && nowPlayerState.pos.y > 0.0f) {
                nowPlayerState.pos.y -= moveSpeed * 0.5f;
                if (nowPlayerState.pos.y < 0.0f) {
                    nowPlayerState.pos.y = 0.0f; // 假设地面在 Y=0
                }
            }

            std::cout << "  New position: " << nowPlayerState.pos.toStr() << std::endl;

        }
        else {
            std::cout << "  Unknown message format." << std::endl;
        }
    }

    // 获取当前游戏状态的字符串表示，用于发送给前端
    std::string GetStateString() const {
        // 协议: STATE:POS=x,y,z
        return "STATE:POS=" + nowPlayerState.pos.toStr();
    }

    // (未来可以添加)
    // void Update(float deltaTime) {
    //     // 处理基于时间的更新，如物理模拟、动画状态等
    // }
};
