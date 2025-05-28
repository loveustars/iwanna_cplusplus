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
    float yOrientationAngle = 0.0f;         // Y轴的旋转角度
    bool isInAir = true; // 初始状态视为在空中
    bool hasWon = false; // 玩家是否已胜利
};

// 代表玩家在当前帧的输入意图
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    float turnInput = 0.0f;
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
    constexpr float SERVER_ROTATION_SPEED = 200.0f;
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
    
    Struct3D playerSize = {
        GameConstants::PLAYER_WIDTH,
        GameConstants::PLAYER_HEIGHT,
        GameConstants::PLAYER_DEPTH
    };
    return CreateAABB(state.pos, playerSize); // Unity默认的
}

// 动态平台
class DynamicPlatform {
public:
    enum class State {
        IDLE,      // 空闲状态，未激活
        ACTIVATED  // 已激活，正在上升
    };

    int id;                 // 唯一ID
    AABB aabb;              // 当前的碰撞盒，会随位置变化
    Struct3D initialCenter; // 初始中心位置
    Struct3D currentCenter; // 当前中心位置
    Struct3D size;          // 平台的尺寸
    float riseSpeed;        // 上升速度
    float maxRiseHeight;    // 最大上升高度
    State state;            // 当前状态

    DynamicPlatform(int p_id, const Struct3D& p_center, const Struct3D& p_size, float p_speed, float p_height)
        : id(p_id), initialCenter(p_center), currentCenter(p_center), size(p_size),
        riseSpeed(p_speed), maxRiseHeight(p_height), state(State::IDLE) {
        // 根据初始中心和尺寸创建初始AABB
        aabb = CreateAABB(currentCenter, size);
    }

    // 更新平台的逻辑，例如移动
    void Update(float deltaTime) {
        if (state == State::ACTIVATED) {
            // 如果已激活且未达到最大高度
            if (currentCenter.y < initialCenter.y + maxRiseHeight) {
                // 向上移动
                currentCenter.y += riseSpeed * deltaTime;
                // 重新计算AABB
                aabb = CreateAABB(currentCenter, size);
            }
        }
    }

    // 激活平台
    void Activate() {
        if (state == State::IDLE) {
            state = State::ACTIVATED;
        }
    }
};

// 新加动态平台
struct MapData {
    std::vector<AABB> obstacles;
    std::vector<DynamicPlatform> dynamicPlatforms;
    Struct3D victoryPoint;
    bool loadedSuccessfully = false;
};

