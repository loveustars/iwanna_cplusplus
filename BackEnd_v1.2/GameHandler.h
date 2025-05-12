// GameHandler.h
#pragma once

#include "3DPos.h"
#include "messages.pb.h" 
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <optional>
#include <fstream>  // 用于文件读取
#include <sstream>  // 用于字符串解析

// 前向声明 AsioNetworkManager，避免循环依赖问题
// AsioNetworkManager.h 中也前向声明了 GameHandler
// 实际的交互逻辑（如 ProcessReceivedData）在 .cpp 文件中实现，可以包含两个头文件
class AsioNetworkManager;


class GameHandler {
public:
    GameHandler(); // 构造函数现在只调用Initialize
    void Initialize(); // 初始化或重置游戏状态，包括加载地图
    void ProcessInput(const PlayerInputState& input);
    void Update(float deltaTime);
    std::optional<std::string> GetStateDataForNetwork() const;

private:
    // 地图加载函数
    MapData LoadMapFromFile(const std::string& filename);
    bool CheckAABBCollision(const AABB& a, const AABB& b) const;
    // 检查玩家是否到达胜利点
    void CheckWinCondition();


    PlayerState nowPlayerState_;
    PlayerInputState currentInput_;
    MapData currentMap_; // 存储当前加载的地图数据
    // std::vector<AABB> obstacles_; // 由 currentMap_.obstacles 替代
    // Struct3D victoryPoint_; // 由 currentMap_.victoryPoint 替代
    // bool playerHasWon_ = false; // 移动到 PlayerState 中
};