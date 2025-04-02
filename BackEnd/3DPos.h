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
};

class PlayerState {
public:
	Struct3D pos;
	bool isInAir = false;

};