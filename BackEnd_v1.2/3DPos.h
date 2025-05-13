#pragma once

#include <string>
#include <cmath>    // ������ѧ����
#include <limits>   // ������ֵ����
#include <compare>  // ����C++20����·�Ƚ������<=>
#include <vector>   // ���ڴ洢���ļ���ȡ��AABB

// ����3D�ռ��еĵ�������Ľṹ��
struct Struct3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // ����������Է��������������
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
    // C++20��·�Ƚ������
    auto operator<=>(const Struct3D&) const = default;

    // �������Struct3D�Ƿ��㹻�ӽ� (����ʤ�������ж�)
    bool IsCloseTo(const Struct3D& other, float tolerance = 0.5f) const {
        return std::abs(x - other.x) < tolerance &&
            std::abs(y - other.y) < tolerance &&
            std::abs(z - other.z) < tolerance;
    }
};

// ������ҵ�����״̬
class PlayerState {
public:
    Struct3D pos = { 0.0f, 0.5f, 0.0f }; // ��ʼλ�ã��Ը��ڵ���
    Struct3D velocity = { 0.0f, 0.0f, 0.0f }; // ��ʼ�ٶ�
    bool isInAir = true; // ��ʼ״̬��Ϊ�ڿ���
    bool hasWon = false; // ����Ƿ���ʤ��
};

// ��������ڵ�ǰ֡��������ͼ
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpPressed = false; // ������Ծ�������µ���һ֡Ϊ true
};

// ��Ϸ��������
namespace GameConstants {
    constexpr float GRAVITY = 9.81f * 2.0f;
    constexpr float MOVE_SPEED = 5.0f;
    constexpr float JUMP_FORCE = 7.5f;
    constexpr float GROUND_LEVEL_Y = 0.0f;
    constexpr float MAX_FALL_VELOCITY = -25.0f;
    constexpr float PLAYER_HEIGHT = 1.0f;
    constexpr float PLAYER_WIDTH = 1.0f;
    constexpr float PLAYER_DEPTH = 1.0f;
    const std::string DEFAULT_MAP_FILE = "map.txt"; // Ĭ�ϵ�ͼ�ļ���
}

// ����һ��������Χ�еĽṹ��
struct AABB {
    Struct3D min;
    Struct3D max;
    auto operator<=>(const AABB&) const = default;
};

// �����������������ĵ�ͳߴ紴��һ�� AABB
inline AABB CreateAABB(const Struct3D& center, const Struct3D& size) {
    Struct3D halfSize = size * 0.5f;
    return {
        {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };
}

// �����������������״̬��ȡ�䵱ǰAABB
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

// �¼��˵�ͼ���ݽṹ�������ϰ����ʤ����
struct MapData {
    std::vector<AABB> obstacles;
    Struct3D victoryPoint;
    bool loadedSuccessfully = false;
};