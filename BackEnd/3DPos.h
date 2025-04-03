#pragma once

#include<iostream>
#include<string>

class Struct3D {
public:
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
	// ת��Ϊ�ַ����Ա㷢�͵�ǰ��
	std::string toStr() const{
		return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
	}
	Struct3D& operator+=(const Struct3D& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Struct3D operator*(float scalar) const {
		return { x * scalar, y * scalar, z * scalar };
	}
};

class PlayerState {
public:
	Struct3D pos;
	bool isInAir = true;
	Struct3D velocity = { 0.0f, 0.0f, 0.0f };

};
const float GRAVITY = 9.81f; // �������ٶȣ����ܻ�ģ�
const float MOVE_SPEED = 5.0f;    // ˮƽ�ƶ��ٶȣ���λ/�룩
const float JUMP_FORCE = 7.0f;    // ����ʱ��˲ʱ��ֱ�ٶȣ���λ/�룩
const float GROUND_LEVEL = 0.0f;  // ����߶� Y ֵ
const float MIN_FALL_VELOCITY = -20.0f; // ��������ٶȣ���ֹ���޼��٣�

// ����״̬
class PlayerInputState {
public:
	bool moveForward = false;
	bool moveBackward = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool jumpPressed = false; // ��Ծ���Ƿ��� (˲�䶯��)
};