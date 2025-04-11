// GameHandler.h
#pragma once

#include "3DPos.h"        // 包含 Struct3D, PlayerState, PlayerInputState, AABB, Constants 等定义
#include "messages.pb.h"  // 包含生成的 Protobuf 消息头文件
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>    // 用于 std::max 等算法
#include <optional>     // 用于 std::optional (例如，序列化可能失败)

// 需要 AsioNetworkManager 的完整定义才能实现 ProcessReceivedData
// 但 AsioNetworkManager 也需要 GameHandler，存在循环依赖。
// 解决方法：将 AsioNetworkManager::ProcessReceivedData 的实现放在 GameHandler.h 底部
// 或者最好是放在一个单独的 .cpp 文件中，并包含两个头文件。
// 这里为了简化，放在 GameHandler.h 底部。
#include "AsioNetworkManager.h"


// 处理核心游戏逻辑、状态更新和碰撞检测的类
class GameHandler {
public:
    // 构造函数: 初始化游戏状态和障碍物
    GameHandler() {
        Initialize(); // 调用初始化函数
        // 添加一些静态障碍物用于测试碰撞
        // 示例: 一个位于 (5, 0.5, 0) 中心，大小为 1x1x1 的立方体障碍物
        obstacles_.push_back(CreateAABB({ 5.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }));
        // 示例: 一个位于 (-2, 1.0, 3) 中心，大小为 1x2x1 的较高障碍物
        obstacles_.push_back(CreateAABB({ -2.0f, 1.0f, 3.0f }, { 1.0f, 2.0f, 1.0f }));
        std::cout << "[Game] GameHandler created and obstacles added." << std::endl;
    }

    // 初始化或重置游戏状态
    void Initialize() {
        // 将玩家状态重置为默认值 (PlayerState 构造函数中定义的)
        nowPlayerState_ = {};
        // 重置当前输入状态
        currentInput_ = {};
        std::cout << "[Game] Game state initialized." << std::endl;
    }

    // 处理从网络层反序列化后传入的、结构化的玩家输入状态
    void ProcessInput(const PlayerInputState& input) {
        // 将接收到的输入状态存储起来，供 Update 函数使用
        currentInput_ = input;
        // 注意：跳跃 (jumpPressed) 的具体效果是在 Update 函数中根据这个状态处理的
    }

    // 根据当前输入和经过的时间 (deltaTime) 来更新游戏状态
    void Update(float deltaTime) {
        // --- 1. 根据输入计算目标水平速度 ---
        // 保留当前的垂直速度，只修改水平速度分量
        Struct3D targetVelocity = { 0.0f, nowPlayerState_.velocity.y, 0.0f };
        // 根据输入标志位累加目标速度
        if (currentInput_.moveForward)  targetVelocity.z += GameConstants::MOVE_SPEED;
        if (currentInput_.moveBackward) targetVelocity.z -= GameConstants::MOVE_SPEED;
        if (currentInput_.moveLeft)   targetVelocity.x -= GameConstants::MOVE_SPEED;
        if (currentInput_.moveRight)  targetVelocity.x += GameConstants::MOVE_SPEED;
        // (未来可以加入奔跑、潜行等改变速度的逻辑)

        // 简单的移动模型：直接设置当前速度为目标速度 (更真实的模型会使用加速度)
        nowPlayerState_.velocity.x = targetVelocity.x;
        nowPlayerState_.velocity.z = targetVelocity.z;

        // --- 2. 处理跳跃 ---
        // 只有当玩家在地面上 (`!isInAir`) 并且当前帧接收到跳跃指令 (`jumpPressed`) 时才执行跳跃
        if (currentInput_.jumpPressed && !nowPlayerState_.isInAir) {
            nowPlayerState_.velocity.y = GameConstants::JUMP_FORCE; // 施加向上的速度冲量
            nowPlayerState_.isInAir = true; // 玩家离开地面，进入空中状态
            std::cout << "[Game] Jump initiated!" << std::endl;
        }
        // 注意: currentInput_.jumpPressed 应该在处理后重置 (见 Update 末尾)，因为它是一个单帧事件


        // --- 3. 应用重力 ---
        // 如果玩家在空中，则应用重力加速度
        if (nowPlayerState_.isInAir) {
            nowPlayerState_.velocity.y -= GameConstants::GRAVITY * deltaTime;
            // 将垂直下落速度限制在最大值，防止速度无限增大
            nowPlayerState_.velocity.y = std::max(nowPlayerState_.velocity.y, GameConstants::MAX_FALL_VELOCITY);
        }
        else {
            // 如果在地面上，垂直速度应为 0 (或者可以处理斜坡逻辑)
            // 可选: 如果在地面且没有水平输入，可以施加地面摩擦力减速
            // if (std::abs(targetVelocity.x) < 0.1f && std::abs(targetVelocity.z) < 0.1f) {
            //     nowPlayerState_.velocity.x *= (1.0f - GROUND_FRICTION * deltaTime);
            //     nowPlayerState_.velocity.z *= (1.0f - GROUND_FRICTION * deltaTime);
            // }
            nowPlayerState_.velocity.y = 0; // 确保在地面时垂直速度归零
        }


        // --- 4. 计算无碰撞时的潜在下一位置 ---
        // 下一位置 = 当前位置 + 速度 * 时间
        Struct3D potentialNextPos = nowPlayerState_.pos + (nowPlayerState_.velocity * deltaTime);

        // --- 5. 碰撞检测与处理 ---

        // 创建一个临时状态对象，用于计算玩家在潜在下一位置的 AABB
        PlayerState nextState = nowPlayerState_;
        nextState.pos = potentialNextPos;
        AABB playerNextAABB = GetPlayerAABB(nextState); // 获取玩家移动后的 AABB

        bool collisionOccurredThisFrame = false; // 标记本帧是否发生任何碰撞

        // 5a. 检测与场景中静态障碍物的碰撞
        for (const auto& obstacle : obstacles_) {
            // 调用 CheckAABBCollision 检查玩家的下一个 AABB 是否与障碍物 AABB 重叠
            if (CheckAABBCollision(playerNextAABB, obstacle)) {
                std::cout << "[Game] Collision detected with obstacle at min{"
                    << obstacle.min.x << "," << obstacle.min.y << "," << obstacle.min.z << "} max{"
                    << obstacle.max.x << "," << obstacle.max.y << "," << obstacle.max.z << "}" << std::endl;
                collisionOccurredThisFrame = true;

                // --- !!! 极其基础的碰撞响应 - 仅作演示 !!! ---
                // 目标: 阻止玩家移动进障碍物。
                // 真正的碰撞响应需要计算穿透深度和方向，然后将玩家推开。
                // 这里的实现非常简化，效果可能不理想，需要后续改进。

                // 查找重叠最小的轴 (作为近似的推出方向) - 这是一种常见的但不完美的简化方法
                // 计算各轴上的重叠量 (如果重叠)
                float overlapX = 0.0f, overlapY = 0.0f, overlapZ = 0.0f;
                if (playerNextAABB.max.x > obstacle.min.x && playerNextAABB.min.x < obstacle.max.x)
                    overlapX = std::min(playerNextAABB.max.x - obstacle.min.x, obstacle.max.x - playerNextAABB.min.x);
                if (playerNextAABB.max.y > obstacle.min.y && playerNextAABB.min.y < obstacle.max.y)
                    overlapY = std::min(playerNextAABB.max.y - obstacle.min.y, obstacle.max.y - playerNextAABB.min.y);
                if (playerNextAABB.max.z > obstacle.min.z && playerNextAABB.min.z < obstacle.max.z)
                    overlapZ = std::min(playerNextAABB.max.z - obstacle.min.z, obstacle.max.z - playerNextAABB.min.z);

                // 优先处理 Y 轴碰撞 (例如，落在物体顶部或撞到物体底部)
                if (overlapY > 0 && overlapY <= overlapX && overlapY <= overlapZ) {
                    if (nowPlayerState_.velocity.y <= 0 && playerNextAABB.min.y < obstacle.max.y) { // 向下运动时撞到顶部 -> 落在上面
                        potentialNextPos.y = obstacle.max.y; // 将玩家 Y 坐标精确放到障碍物顶部
                        nowPlayerState_.velocity.y = 0;      // 停止垂直速度
                        nowPlayerState_.isInAir = false;     // 玩家现在站在障碍物上
                        std::cout << "[Game] Landed on obstacle." << std::endl;
                    }
                    else if (nowPlayerState_.velocity.y > 0 && playerNextAABB.max.y > obstacle.min.y) { // 向上运动时撞到底部
                        potentialNextPos.y = obstacle.min.y - GameConstants::PLAYER_HEIGHT; // 放到障碍物下方
                        nowPlayerState_.velocity.y = 0; // 停止向上的速度 (撞头)
                        std::cout << "[Game] Hit obstacle underside." << std::endl;
                    }
                }
                // 其次处理 X 轴碰撞 (左右碰撞)
                else if (overlapX > 0 && overlapX <= overlapY && overlapX <= overlapZ) {
                    float pushBackEpsilon = 0.001f; // 加一点微小的偏移防止卡住
                    if (nowPlayerState_.velocity.x > 0 && playerNextAABB.max.x > obstacle.min.x) { // 向右运动撞到左侧面
                        potentialNextPos.x = obstacle.min.x - GameConstants::PLAYER_WIDTH * 0.5f - pushBackEpsilon; // 移到障碍物左侧
                        nowPlayerState_.velocity.x = 0; // 停止 X 轴速度
                    }
                    else if (nowPlayerState_.velocity.x < 0 && playerNextAABB.min.x < obstacle.max.x) { // 向左运动撞到右侧面
                        potentialNextPos.x = obstacle.max.x + GameConstants::PLAYER_WIDTH * 0.5f + pushBackEpsilon; // 移到障碍物右侧
                        nowPlayerState_.velocity.x = 0; // 停止 X 轴速度
                    }
                    std::cout << "[Game] Hit obstacle side (X)." << std::endl;
                }
                // 最后处理 Z 轴碰撞 (前后碰撞)
                else if (overlapZ > 0) {
                    float pushBackEpsilon = 0.001f;
                    if (nowPlayerState_.velocity.z > 0 && playerNextAABB.max.z > obstacle.min.z) { // 向前运动撞到后侧面
                        potentialNextPos.z = obstacle.min.z - GameConstants::PLAYER_DEPTH * 0.5f - pushBackEpsilon; // 移到障碍物后面
                        nowPlayerState_.velocity.z = 0; // 停止 Z 轴速度
                    }
                    else if (nowPlayerState_.velocity.z < 0 && playerNextAABB.min.z < obstacle.max.z) { // 向后运动撞到前侧面
                        potentialNextPos.z = obstacle.max.z + GameConstants::PLAYER_DEPTH * 0.5f + pushBackEpsilon; // 移到障碍物前面
                        nowPlayerState_.velocity.z = 0; // 停止 Z 轴速度
                    }
                    std::cout << "[Game] Hit obstacle side (Z)." << std::endl;
                }

                // 重要: 在处理完一个碰撞后，理想情况下应该重新计算 AABB 并再次检查与其他物体的碰撞。
                // 当前简化实现是基于初始 potentialNextPos 对所有障碍物进行检查和调整。
                // 这可能导致在复杂情况下（如角落）响应不精确。
            }
        }


        // 5b. 检测与地面的碰撞
        // (未来可以把地面也视为一个巨大的 AABB 来统一处理)
        // 当前逻辑：如果玩家潜在位置低于或等于地面 Y 坐标，并且正在下落或静止
        bool landedOnGroundThisFrame = false; // 标记本帧是否着陆
        if (potentialNextPos.y <= GameConstants::GROUND_LEVEL_Y && nowPlayerState_.velocity.y <= 0) {
            // 如果玩家已经在地面上，只需确保 Y 坐标不低于地面
            if (!nowPlayerState_.isInAir) {
                potentialNextPos.y = GameConstants::GROUND_LEVEL_Y;
            }
            // 如果玩家刚接触地面
            else {
                potentialNextPos.y = GameConstants::GROUND_LEVEL_Y; // 将 Y 坐标精确设置到地面
                nowPlayerState_.velocity.y = 0;      // 停止垂直下落速度
                nowPlayerState_.isInAir = false;    // 标记为不在空中 (已着陆)
                landedOnGroundThisFrame = true;     // 标记本帧发生了着陆
                std::cout << "[Game] Landed on ground." << std::endl;
            }
            collisionOccurredThisFrame = true; // 将接触地面也视为一种碰撞
        }

        // --- 6. 更新最终位置 ---
        // 应用经过碰撞检测和响应调整后的位置
        nowPlayerState_.pos = potentialNextPos;

        // --- 7. 更新空中状态 ---
        // 如果在本帧的碰撞处理中没有确定玩家着陆 (在障碍物或地面上)，
        // 并且玩家的最终位置高于地面，那么玩家应该处于空中状态。
        if (!landedOnGroundThisFrame && !(!nowPlayerState_.isInAir && collisionOccurredThisFrame)) { // 如果没着陆，且不是被其他碰撞强行置于地面
            if (nowPlayerState_.pos.y > GameConstants::GROUND_LEVEL_Y) {
                // 如果之前在地上但现在位置高于地面 (比如从边缘走下)
                if (!nowPlayerState_.isInAir) {
                    std::cout << "[Game] Left ground." << std::endl;
                }
                nowPlayerState_.isInAir = true; // 标记为在空中
            }
            // else (如果位置等于地面Y但没有速度，应该已经在上面被置为 false 了)
        }


        // --- 8. 重置单帧输入事件 ---
        // 跳跃指令只在按下的那一帧有效，处理完后需要重置
        currentInput_.jumpPressed = false;

    }

    // 将当前游戏状态序列化为 Protobuf 二进制字符串，用于网络发送
    // 如果序列化失败，返回 std::nullopt
    std::optional<std::string> GetStateDataForNetwork() const {
        // 1. 创建顶层消息对象 ServerToClient
        game_backend::ServerToClient server_msg;
        // 2. 获取顶层消息中类型为 GameState 的可变负载部分
        //    (因为我们在 .proto 定义了 oneof payload { GameState state = 1; })
        game_backend::GameState* state_payload = server_msg.mutable_state();

        // --- 序列化使用点 (填充 Protobuf 对象) ---
        // 3. 填充 GameState 负载中的数据
        // 3a. 填充位置信息
        game_backend::Vector3* pos_proto = state_payload->mutable_position(); // 获取位置字段的可变指针
        pos_proto->set_x(nowPlayerState_.pos.x);
        pos_proto->set_y(nowPlayerState_.pos.y);
        pos_proto->set_z(nowPlayerState_.pos.z);

        // 3b. 填充速度信息
        game_backend::Vector3* vel_proto = state_payload->mutable_velocity(); // 获取速度字段的可变指针
        vel_proto->set_x(nowPlayerState_.velocity.x);
        vel_proto->set_y(nowPlayerState_.velocity.y);
        vel_proto->set_z(nowPlayerState_.velocity.z);

        // 3c. 填充其他状态信息
        state_payload->set_is_in_air(nowPlayerState_.isInAir);
        // (未来在这里填充其他需要同步的状态...)

        // 4. 将填充好的顶层消息对象 (server_msg) 序列化为二进制字符串
        std::string serialized_data;
        if (!server_msg.SerializeToString(&serialized_data)) {
            // 如果序列化失败
            std::cerr << "[Game] Error: Failed to serialize game state!" << std::endl;
            return std::nullopt; // 返回空的 optional 表示失败
        }

        // 5. 返回包含序列化后二进制数据的字符串
        return serialized_data;
    }

private:
    // 检查两个 AABB 是否发生碰撞 (重叠)
    // const 成员函数，因为它不修改 GameHandler 的状态
    bool CheckAABBCollision(const AABB& a, const AABB& b) const {
        // 在所有三个轴上都存在重叠，才算发生碰撞
        // 检查 X 轴是否有重叠 (a 的右边界 > b 的左边界 AND a 的左边界 < b 的右边界)
        bool xOverlap = a.max.x > b.min.x && a.min.x < b.max.x;
        // 检查 Y 轴是否有重叠
        bool yOverlap = a.max.y > b.min.y && a.min.y < b.max.y;
        // 检查 Z 轴是否有重叠
        bool zOverlap = a.max.z > b.min.z && a.min.z < b.max.z;

        // 只有当三个轴都重叠时，才判定为碰撞
        return xOverlap && yOverlap && zOverlap;
    }


    // 存储当前的玩家状态
    PlayerState nowPlayerState_;
    // 存储当前帧处理的玩家输入意图
    PlayerInputState currentInput_;
    // 存储关卡中的静态障碍物 (AABB 列表)
    std::vector<AABB> obstacles_;

    // --- 以下是被移除或替代的旧函数 ---
    // TransKeyString (被 Protobuf 反序列化替代)
    // GetStateString (被 GetStateDataForNetwork 替代)
    // ProcessMessage (被 ProcessInput 和 Protobuf 反序列化替代)
};
