// 3DPos.h
#pragma once

#include <string>
#include <cmath>    // ������ѧ���������� std::sqrt (����Ժ���Ҫ)
#include <limits>   // ������ֵ���� (����Ժ���Ҫ)
#include <compare>  // ���� C++20 ����·�Ƚ������ <=>

// ���� 3D �ռ��еĵ�������Ľṹ��
struct Struct3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // �Ƴ��� toStr() ��������Ϊ���л����� Protobuf ����

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

    // ����������ĳ˷�
    Struct3D operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    // C++20: Ĭ��ʵ�ֵ���·�Ƚ������ (spaceship operator)
    // ��ʹ�� Struct3D �������ֱ��ʹ�� ==, !=, <, >, <=, >= ���бȽ�
    auto operator <=> (const Struct3D&) const = default;
};

// ������ҵ�����״̬
class PlayerState {
public:
    Struct3D pos = { 0.0f, 0.5f, 0.0f }; // ��ʼλ�ã��Ը��ڵ���
    Struct3D velocity = { 0.0f, 0.0f, 0.0f }; // ��ʼ�ٶ�
    bool isInAir = true; // ��ʼ״̬��Ϊ�ڿ��� (��Ҫ���䵽����)
    // ֮�������Ӹ���״̬���糯������ֵ��
};

// ��������ڵ�ǰ֡��������ͼ
// (ͨ��ͨ�������л�������Ϣ�õ�)
struct PlayerInputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jumpPressed = false; // ������Ծ�������µ���һ֡Ϊ true
};


// ��Ϸ�������� (ʹ�� constexpr ȷ������ʱ��ֵ)
namespace GameConstants {
    constexpr float GRAVITY = 9.81f * 2.0f; // �������ٶ� (���� 2 �������������"��Ϸ��")
    constexpr float MOVE_SPEED = 5.0f;      // ���ˮƽ�ƶ��ٶ�
    constexpr float JUMP_FORCE = 7.5f;      // �����Ծ�ĳ�ʼ��ֱ�ٶ� (����)
    constexpr float GROUND_LEVEL_Y = 0.0f;  // ����� Y ����
    constexpr float MAX_FALL_VELOCITY = -25.0f; // ��������ٶ� (�����ն��ٶ�)
    constexpr float PLAYER_HEIGHT = 1.0f;   // ��Ҹ߶� (������ײ/AABB)
    constexpr float PLAYER_WIDTH = 1.0f;    // ��ҿ�� (������ײ/AABB)
    constexpr float PLAYER_DEPTH = 1.0f;    // ������ (������ײ/AABB)
    // ֮����Ը�����Ҫ��Ӹ��ೣ�� (����: ����Ħ��ϵ��, ��������ϵ����)
}

// ����һ��������Χ�� (Axis-Aligned Bounding Box) �Ľṹ��
struct AABB {
    Struct3D min; // ��Χ�е���С������ (x_min, y_min, z_min)
    Struct3D max; // ��Χ�е��������� (x_max, y_max, z_max)

    // C++20: Ĭ�ϱȽ������
    auto operator<=>(const AABB&) const = default;
};

// �����������������ĵ�ͳߴ紴��һ�� AABB
inline AABB CreateAABB(const Struct3D& center, const Struct3D& size) {
    Struct3D halfSize = size * 0.5f; // �����ߴ�
    return {
        // ��С�� = ���� - ��ߴ�
        {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
        // ���� = ���� + ��ߴ�
        {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
    };
}

// �����������������״̬��ȡ�䵱ǰ�� AABB
inline AABB GetPlayerAABB(const PlayerState& state) {
    // ������ҵ�λ�� state.pos ����ŵ׵����ĵ�
    // ������� AABB �����ĵ� Y ����
    Struct3D playerCenter = {
        state.pos.x,
        state.pos.y + GameConstants::PLAYER_HEIGHT * 0.5f, // ���� Y = λ�� Y + ���
        state.pos.z
    };
    // ��ȡ��ҵĳߴ�
    Struct3D playerSize = {
        GameConstants::PLAYER_WIDTH,
        GameConstants::PLAYER_HEIGHT,
        GameConstants::PLAYER_DEPTH
    };
    // ʹ�� CreateAABB ���������� AABB
    return CreateAABB(playerCenter, playerSize);
}