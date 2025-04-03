#pragma once
#include <string>
#include <iostream>
#include <sstream> // 解析字符串
#include <vector>
#include <algorithm>
#include <map>     // 存储输入
#include "3DPos.h"


class GameHandler {
private:
    PlayerState nowPlayerState;
    PlayerInputState nowInput;

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
        nowPlayerState.pos = { 0.0f, 0.5f, 0.0f};
        nowPlayerState.velocity = { 0.0f, 0.0f, 0.0f };
        nowPlayerState.isInAir = true;
        nowInput = {};
        std::cout << "GameManager initialized. Player start position: "
            << nowPlayerState.pos.toStr() << std::endl;
    }

    // 处理从网络接收到的消息
    void ProcessMessage(const std::string& message) {
        nowInput.jumpPressed = false;
        // 简单的协议解析: 查找 "INPUT:" 前缀
        if (message.rfind("INPUT:", 0) == 0) { // 找INPUT开头的消息
            std::string inputData = message.substr(6); // 获取冒号之后的部分
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
        // (可选) 如果需要更平滑的移动，可以使用插值或加速度来改变速度，而不是直接设置
        // 这里为了简单，直接设置目标速度
        nowPlayerState.velocity.x = tgVelocity.x;
        nowPlayerState.velocity.z = tgVelocity.z;

        // 2. 处理跳跃 (状态机逻辑: 只有在地面才能跳)
        if (nowInput.jumpPressed && !nowPlayerState.isInAir) {
            nowPlayerState.velocity.y = JUMP_FORCE; // 施加向上的力
            nowPlayerState.isInAir = true;      // 离开地面
            std::cout << "Jump!" << std::endl;
        }
        // 重置跳跃键状态，确保只跳一次
        nowInput.jumpPressed = false;


        // 3. 应用重力 (只在空中时)
        if (nowPlayerState.isInAir) {
            nowPlayerState.velocity.y -= GRAVITY * deltaTime;
            // 限制最大下落速度
            nowPlayerState.velocity.y = max(nowPlayerState.velocity.y, MIN_FALL_VELOCITY);
        }


        // 4. 更新位置 (根据最终速度）
        nowPlayerState.pos += nowPlayerState.velocity * deltaTime;


        // 5. 碰撞检测（目前只有地面）
        //  --- 这是最基础的碰撞检测 ---
        bool prevAir = nowPlayerState.isInAir; // 记录之前的状态

        if (nowPlayerState.pos.y <= GROUND_LEVEL && nowPlayerState.velocity.y <= 0) {
            // 如果玩家位置低于或等于地面，并且正在下落或静止
            nowPlayerState.pos.y = GROUND_LEVEL; // 修正位置到地面
            nowPlayerState.velocity.y = 0;            // 停止垂直速度
            nowPlayerState.isInAir = false;         // 标记为在地面上
            if (!prevAir) {
                std::cout << "Landed." << std::endl;
            }
        }
        else {
            // 如果玩家在空中
            if (prevAir && nowPlayerState.velocity.y != 0) { // 从地面起跳或掉落
                std::cout << "Left ground." << std::endl;
            }
            nowPlayerState.isInAir = true; // 不在地面
        }

        // --- 真实游戏需要更复杂的碰撞检测，涉及关卡几何体 ---

    }

    // 获取当前游戏状态的字符串表示，用于发送给前端
    std::string GetStateString() const {
        // 注意协议: STATE:POS=x,y,z;VEL=vx,vy,vz;AIR=0/1
        std::string stateStr = "STATE:";
        stateStr += "POS=" + nowPlayerState.pos.toStr();
        stateStr += ";V=" + nowPlayerState.velocity.toStr();
        stateStr += ";AIR=" + std::string(nowPlayerState.isInAir ? "1" : "0");
        return stateStr;
    }

};
