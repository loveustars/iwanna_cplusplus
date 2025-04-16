// 3DPos.h
#pragma once

#include <string>
#include <cmath>    // 用于数学函数
#include <limits>   // 用于数值极限
#include <compare>  // 用于C++20的三路比较运算符<=>

// 代表3D空间中的点或向量的结构体
struct Struct3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    // 移除toStr()函数，因为序列化将由Protobuf处理
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
    auto operator<=>(const Struct3D&) const = default;
};
// 代表玩家的物理状态
class PlayerState {
public:
    Struct3D pos = { 0.0f, 0.5f, 0.0f }; // 初始位置，略高于地面
    Struct3D velocity = { 0.0f, 0.0f, 0.0f }; // 初始速度
    bool isInAir = true; // 初始状态视为在空中
};

// 代表玩家在当前帧的输入意图
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpPressed = false; // 仅在跳跃键被按下的那一帧为 true
};
// 游戏常量定义（constexpr确保编译时求值）
namespace GameConstants {
    constexpr float GRAVITY = 9.81f * 2.0f; // 重力加速度 (乘以 2 可能让下落更有"游戏感")
    constexpr float MOVE_SPEED = 5.0f;      // 玩家水平移动速度
    constexpr float JUMP_FORCE = 7.5f;      // 玩家跳跃的初始垂直速度 (冲量)
    constexpr float GROUND_LEVEL_Y = 0.0f;  // 地面的 Y 坐标
    constexpr float MAX_FALL_VELOCITY = -25.0f; // 最大下落速度 (限制终端速度)
    constexpr float PLAYER_HEIGHT = 1.0f;   // 玩家高度 (用于碰撞/AABB)
    constexpr float PLAYER_WIDTH = 1.0f;    // 玩家宽度 (用于碰撞/AABB)
    constexpr float PLAYER_DEPTH = 1.0f;    // 玩家深度 (用于碰撞/AABB)
    // 之后可以根据需要添加更多常量 (例如: 地面摩擦系数, 空气阻力系数等)
}

// 代表一个轴对齐包围盒的结构体
struct AABB {
    Struct3D min; // 包围盒的最小角坐标 (x_min, y_min, z_min)
    Struct3D max; // 包围盒的最大角坐标 (x_max, y_max, z_max)

    // C++20: 默认比较运算符
    auto operator<=>(const AABB&) const = default;
};
// 辅助函数：根据中心点和尺寸创建一个 AABB
inline AABB CreateAABB(const Struct3D& center, const Struct3D& size) {
    Struct3D halfSize = size * 0.5f; // 计算半尺寸
    return {
        {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };
}

// 辅助函数：根据玩家状态获取其当前AABB
inline AABB GetPlayerAABB(const PlayerState& state) {
    // 假设玩家的state.pos是其脚底的中心点
    // 计算玩家AABB的中心点Y坐标
    Struct3D playerCenter = {
        state.pos.x,
        state.pos.y + GameConstants::PLAYER_HEIGHT * 0.5f,
        state.pos.z
    };
    // 获取玩家的尺寸
    Struct3D playerSize = {
        GameConstants::PLAYER_WIDTH,
        GameConstants::PLAYER_HEIGHT,
        GameConstants::PLAYER_DEPTH
    };
    // 返回创建的AABB
    return CreateAABB(playerCenter, playerSize);
}