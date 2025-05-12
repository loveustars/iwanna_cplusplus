// GameHandler.h
#pragma once

#include "3DPos.h"
#include "messages.pb.h" 
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <fstream>  // �����ļ���ȡ
#include <sstream>  // �����ַ�������

// ǰ������ AsioNetworkManager������ѭ����������
// AsioNetworkManager.h ��Ҳǰ�������� GameHandler
// ʵ�ʵĽ����߼����� ProcessReceivedData���� .cpp �ļ���ʵ�֣����԰�������ͷ�ļ�
class AsioNetworkManager;


class GameHandler {
public:
    GameHandler(); // ���캯������ֻ����Initialize
    void Initialize(); // ��ʼ����������Ϸ״̬���������ص�ͼ
    void ProcessInput(const PlayerInputState& input);
    void Update(float deltaTime);
    std::optional<std::string> GetStateDataForNetwork() const;

private:
    // ��ͼ���غ���
    MapData LoadMapFromFile(const std::string& filename);
    bool CheckAABBCollision(const AABB& a, const AABB& b) const;
    // �������Ƿ񵽴�ʤ����
    void CheckWinCondition();


    PlayerState nowPlayerState_;
    PlayerInputState currentInput_;
    MapData currentMap_; // �洢��ǰ���صĵ�ͼ����
    // std::vector<AABB> obstacles_; // �� currentMap_.obstacles ���
    // Struct3D victoryPoint_; // �� currentMap_.victoryPoint ���
    // bool playerHasWon_ = false; // �ƶ��� PlayerState ��
};