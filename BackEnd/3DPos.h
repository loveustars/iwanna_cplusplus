#pragma once

#include<iostream>
#include<string>

class Struct3D {
public:
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
	// 转换为字符串以便发送到前端
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
const float GRAVITY = 9.81f; // 重力加速度（可能会改）
const float MOVE_SPEED = 5.0f;    // 水平移动速度（单位/秒）
const float JUMP_FORCE = 7.0f;    // 起跳时的瞬时垂直速度（单位/秒）
const float GROUND_LEVEL = 0.0f;  // 地面高度 Y 值
const float MIN_FALL_VELOCITY = -20.0f; // 最大下落速度（防止无限加速）

// 输入状态
class PlayerInputState {
public:
	bool moveForward = false;
	bool moveBackward = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool jumpPressed = false; // 跳跃键是否按下 (瞬间动作)
};