// 3DPos.h
#pragma once

#include <string>
#include <cmath>    // 用于数学函数，例如 std::sqrt (如果以后需要)
#include <limits>   // 用于数值极限 (如果以后需要)
#include <compare>  // 用于 C++20 的三路比较运算符 <=>

// 代表 3D 空间中的点或向量的结构体
struct Struct3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // 移除了 toStr() 函数，因为序列化将由 Protobuf 处理

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

    // 向量与标量的乘法
    Struct3D operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    // C++20: 默认实现的三路比较运算符 (spaceship operator)
    // 这使得 Struct3D 对象可以直接使用 ==, !=, <, >, <=, >= 进行比较
    auto operator <=> (const Struct3D&) const = default;
};

// 代表玩家的物理状态
class PlayerState {
public:
    Struct3D pos = { 0.0f, 0.5f, 0.0f }; // 初始位置，略高于地面
    Struct3D velocity = { 0.0f, 0.0f, 0.0f }; // 初始速度
    bool isInAir = true; // 初始状态视为在空中 (需要下落到地面)
    // 之后可以添加更多状态，如朝向、生命值等
};

// 代表玩家在当前帧的输入意图
// (通常通过反序列化网络消息得到)
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpPressed = false; // 仅在跳跃键被按下的那一帧为 true
};


// 游戏常量定义 (使用 constexpr 确保编译时求值)
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

// 代表一个轴对齐包围盒 (Axis-Aligned Bounding Box) 的结构体
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
        // 最小角 = 中心 - 半尺寸
        {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        // 最大角 = 中心 + 半尺寸
        {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };
}

// 辅助函数：根据玩家状态获取其当前的 AABB
inline AABB GetPlayerAABB(const PlayerState& state) {
    // 假设玩家的位置 state.pos 是其脚底的中心点
    // 计算玩家 AABB 的中心点 Y 坐标
    Struct3D playerCenter = {
        state.pos.x,
        state.pos.y + GameConstants::PLAYER_HEIGHT * 0.5f, // 中心 Y = 位置 Y + 半高
        state.pos.z
    };
    // 获取玩家的尺寸
    Struct3D playerSize = {
        GameConstants::PLAYER_WIDTH,
        GameConstants::PLAYER_HEIGHT,
        GameConstants::PLAYER_DEPTH
    };
    // 使用 CreateAABB 创建并返回 AABB
    return CreateAABB(playerCenter, playerSize);
}