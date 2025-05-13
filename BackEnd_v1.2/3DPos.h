#pragma once

#include <string>
#include <cmath>    // 用于数学函数
#include <limits>   // 用于数值极限
#include <compare>  // 用于C++20的三路比较运算符<=>
#include <vector>   // 用于存储从文件读取的AABB

// 代表3D空间中的点或向量的结构体
struct Struct3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // 重载运算符以方便进行向量运算
    Struct3D& operator+=(const Struct3D& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }
    Struct3D& operator-=(const Struct3D& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }
    Struct3D operator+(const Struct3D& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }
    Struct3D operator-(const Struct3D& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }
    Struct3D operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }
    // C++20三路比较运算符
    auto operator<=>(const Struct3D&) const = default;

    // 检查两个Struct3D是否足够接近 (用于胜利条件判断)
    bool IsCloseTo(const Struct3D& other, float tolerance = 0.5f) const {
        return std::abs(x - other.x) < tolerance &&
            std::abs(y - other.y) < tolerance &&
            std::abs(z - other.z) < tolerance;
    }
};

// 代表玩家的物理状态
class PlayerState {
public:
    Struct3D pos = { 0.0f, 0.5f, 0.0f }; // 初始位置，略高于地面
    Struct3D velocity = { 0.0f, 0.0f, 0.0f }; // 初始速度
    bool isInAir = true; // 初始状态视为在空中
    bool hasWon = false; // 玩家是否已胜利
};

// 代表玩家在当前帧的输入意图
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpPressed = false; // 仅在跳跃键被按下的那一帧为 true
};

// 游戏常量定义
namespace GameConstants {
    constexpr float GRAVITY = 9.81f * 2.0f;
    constexpr float MOVE_SPEED = 5.0f;
    constexpr float JUMP_FORCE = 7.5f;
    constexpr float GROUND_LEVEL_Y = 0.0f;
    constexpr float MAX_FALL_VELOCITY = -25.0f;
    constexpr float PLAYER_HEIGHT = 1.0f;
    constexpr float PLAYER_WIDTH = 1.0f;
    constexpr float PLAYER_DEPTH = 1.0f;
    const std::string DEFAULT_MAP_FILE = "map.txt"; // 默认地图文件名
}

// 代表一个轴对齐包围盒的结构体
struct AABB {
    Struct3D min;
    Struct3D max;
    auto operator<=>(const AABB&) const = default;
};

// 辅助函数：根据中心点和尺寸创建一个 AABB
inline AABB CreateAABB(const Struct3D& center, const Struct3D& size) {
    Struct3D halfSize = size * 0.5f;
    return {
        {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };
}

// 辅助函数：根据玩家状态获取其当前AABB
inline AABB GetPlayerAABB(const PlayerState& state) {
    Struct3D playerCenter = {
        state.pos.x,
        state.pos.y + GameConstants::PLAYER_HEIGHT * 0.5f,
        state.pos.z
    };
    Struct3D playerSize = {
        GameConstants::PLAYER_WIDTH,
        GameConstants::PLAYER_HEIGHT,
        GameConstants::PLAYER_DEPTH
    };
    return CreateAABB(playerCenter, playerSize);
}

// 新加了地图数据结构，包含障碍物和胜利点
struct MapData {
    std::vector<AABB> obstacles;
    Struct3D victoryPoint;
    bool loadedSuccessfully = false;
};