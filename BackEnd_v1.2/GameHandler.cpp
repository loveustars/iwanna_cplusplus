// GameHandler.cpp

#include "GameHandler.h"
#include "AsioNetworkManager.h"
// 确保 AsioNetworkManager 的完整定义可见

// GameHandler 构造函数
GameHandler::GameHandler() {
    Initialize(); // 调用初始化函数，该函数将加载地图
}

// 初始化游戏状态，包括从文件加载地图数据
void GameHandler::Initialize() {
    nowPlayerState_ = {}; // 重置玩家状态
    currentInput_ = {};   // 重置输入状态

    // 加载地图数据
    currentMap_ = LoadMapFromFile(GameConstants::DEFAULT_MAP_FILE);
    if (!currentMap_.loadedSuccessfully) {
        std::cerr << "[Game] Error: Failed to load map data from " << GameConstants::DEFAULT_MAP_FILE
            << ". Using empty map." << std::endl;
        // 可以选择加载一个默认的空地图或者抛出异常
        currentMap_.obstacles.clear();
        currentMap_.victoryPoint = { 0.0f, 0.0f, 10.0f }; // 设置一个默认胜利点以防万一
    }
    else {
        std::cout << "[Game] Map data loaded successfully from " << GameConstants::DEFAULT_MAP_FILE << "." << std::endl;
        std::cout << "[Game] Loaded " << currentMap_.obstacles.size() << " obstacles." << std::endl;
        std::cout << "[Game] Victory point set to: ("
            << currentMap_.victoryPoint.x << ", "
            << currentMap_.victoryPoint.y << ", "
            << currentMap_.victoryPoint.z << ")." << std::endl;
    }
    std::cout << "[Game] Game state initialized/reset." << std::endl;
}


// 从文件加载地图数据
// 文件格式:
// 第一行: victory_point x y z
// 之后每行: obstacle_aabb min_x min_y min_z max_x max_y max_z

/*
MapData GameHandler::LoadMapFromFile(const std::string& filename) {
    MapData mapData;
    mapData.loadedSuccessfully = false; // 默认加载失败
    std::ifstream mapFile(filename);
    std::string line;

    if (!mapFile.is_open()) {
        std::cerr << "[Game] Error: Could not open map file: " << filename << std::endl;
        return mapData; // 返回加载失败的MapData
    }

    // 读取胜利点
    if (std::getline(mapFile, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "victory_point") {
            iss >> mapData.victoryPoint.x >> mapData.victoryPoint.y >> mapData.victoryPoint.z;
        }
        else {
            std::cerr << "[Game] Error: Invalid map file format. Expected 'victory_point' on the first line." << std::endl;
            mapFile.close();
            return mapData; // 返回加载失败的MapData
        }
    }
    else {
        std::cerr << "[Game] Error: Map file is empty or could not read victory point." << std::endl;
        mapFile.close();
        return mapData; // 返回加载失败的MapData
    }

    // 读取障碍物
    while (std::getline(mapFile, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "obstacle_aabb") {
            AABB obstacle;
            iss >> obstacle.min.x >> obstacle.min.y >> obstacle.min.z
                >> obstacle.max.x >> obstacle.max.y >> obstacle.max.z;
            mapData.obstacles.push_back(obstacle);
        }
        else if (!line.empty() && line[0] != '#') { // 忽略空行和注释行
            std::cerr << "[Game] Warning: Skipping invalid line in map file: " << line << std::endl;
        }
    }

    mapFile.close();
    mapData.loadedSuccessfully = true; // 标记加载成功
    return mapData;
}*/

MapData GameHandler::LoadMapFromFile(const std::string& filename) {
    MapData mapData;
    mapData.loadedSuccessfully = false; // 默认加载失败
    std::ifstream mapFile(filename);
    std::string line;
    if (!mapFile.is_open()) {
        std::cerr << "[Game] Error: Could not open map file: " << filename << std::endl;
        return mapData;
    }

    // 读取胜利点
    if (std::getline(mapFile, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "victory_point") {
            iss >> mapData.victoryPoint.x >> mapData.victoryPoint.y >> mapData.victoryPoint.z;
        }
        else {
            std::cerr << "[Game] Error: Invalid map file format. Expected 'victory_point' on the first line." << std::endl;
            mapFile.close();
            return mapData;
        }
    }
    else {
        std::cerr << "[Game] Error: Map file is empty or could not read victory point." << std::endl;
        mapFile.close();
        return mapData;
    }

    // 读取障碍物
    while (std::getline(mapFile, line)) {
        // 修改:将检查注释和空行的逻辑放在最前面
        // 1. 去除行首的空白字符，使判断更稳健
        size_t first_char_pos = line.find_first_not_of(" \t");

        // 2. 如果这一行是空的，或者第一个非空白字符是'#'，则跳过这一行
        if (line.empty() || first_char_pos == std::string::npos || line[first_char_pos] == '#') {
            continue;
        }

        // 3. 只有在确认不是注释或空行后，才进行解析
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "obstacle_aabb") {
            AABB obstacle;
            // 增加了解析数字是否成功的检查
            if (!(iss >> obstacle.min.x >> obstacle.min.y >> obstacle.min.z
                >> obstacle.max.x >> obstacle.max.y >> obstacle.max.z)) {
                std::cerr << "[Game] Warning: Malformed 'obstacle_aabb' line (could not parse numbers): " << line << std::endl;
                continue;
            }
            mapData.obstacles.push_back(obstacle);
        }
        else {
            // 4. 如果行首的单词不是我们认识的任何类型，则报告为无效行
            std::cerr << "[Game] Warning: Skipping unrecognized line type in map file: " << line << std::endl;
        }
    }

    mapFile.close();
    mapData.loadedSuccessfully = true; // 标记加载成功
    return mapData;
}



void GameHandler::ProcessInput(const PlayerInputState& input) {
    currentInput_ = input;
}

void GameHandler::Update(float deltaTime) {
    // 如果玩家已经胜利，则不更新游戏逻辑
    if (nowPlayerState_.hasWon) {
        // 可以选择重置跳跃输入，防止在胜利后还能收到跳跃指令影响状态
        currentInput_.jumpPressed = false;
        return;
    }

    /*
    nowPlayerState_.velocity.x = 0.0f;
    nowPlayerState_.velocity.z = 0.0f;

    // 1. 根据输入计算目标水平速度
    if (currentInput_.moveForward)  nowPlayerState_.velocity.z += GameConstants::MOVE_SPEED;
    if (currentInput_.moveBackward) nowPlayerState_.velocity.z -= GameConstants::MOVE_SPEED;
    if (currentInput_.moveLeft)   nowPlayerState_.velocity.x -= GameConstants::MOVE_SPEED;
    if (currentInput_.moveRight)  nowPlayerState_.velocity.x += GameConstants::MOVE_SPEED;
   */

   // --- 0. 更新玩家Y轴旋转 (根据 turnInput) ---
    if (currentInput_.turnInput != 0.0f) {
        float rotationAmount = currentInput_.turnInput * GameConstants::SERVER_ROTATION_SPEED * deltaTime;
        nowPlayerState_.yOrientationAngle += rotationAmount;

        // 可选：将角度标准化到 0-360 度范围，避免数值无限增大
        // nowPlayerState_.yOrientationAngle = fmod(fmod(nowPlayerState_.yOrientationAngle, 360.0f) + 360.0f, 360.0f);
        // 或者标准化到 -180 到 180
        while (nowPlayerState_.yOrientationAngle > 180.0f) nowPlayerState_.yOrientationAngle -= 360.0f;
        while (nowPlayerState_.yOrientationAngle < -180.0f) nowPlayerState_.yOrientationAngle += 360.0f;
    }
    // -----------------------------------------

    // 1. 根据输入计算目标世界坐标系下的速度 (W/S 控制Z轴, A/D 控制X轴 - 这是世界坐标系平移)
    // 如果你希望W/S是“向前/向后”（相对于角色朝向），A/D是“向左/向右旋转”，那么速度计算会不同。
    // 当前代码：W/S控制世界Z，A/D(moveLeft/Right)控制世界X。TurnInput只控制旋转。
    nowPlayerState_.velocity.x = 0.0f;
    nowPlayerState_.velocity.y = nowPlayerState_.velocity.y; // 保留垂直速度 (重力和跳跃)***
    nowPlayerState_.velocity.z = 0.0f;

    // --- 如果W/S是相对于角色朝向的移动 ---
    // 这部分需要根据你的 PlayerInputState 中 moveLeft/Right 的含义来决定
    // 假设:
    // - currentInput_.moveForward/Backward 用于前后移动
    // - currentInput_.moveLeft/Right 用于左右平移 (strafe)
    // - currentInput_.turnInput 用于原地旋转 (我们已经在上面处理了旋转本身)

    Struct3D moveDirection = { 0,0,0 };
    if (currentInput_.moveForward)  moveDirection.z += 1.0f;
    if (currentInput_.moveBackward) moveDirection.z -= 1.0f;
    if (currentInput_.moveRight)    moveDirection.x += 1.0f;
    if (currentInput_.moveLeft)     moveDirection.x -= 1.0f;

    if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
        // 将本地移动方向 (基于输入) 旋转到世界坐标系
        float angleRad = nowPlayerState_.yOrientationAngle * (3.1415926535f / 180.0f); // 转为弧度
        float cosAngle = std::cos(angleRad);
        float sinAngle = - std::sin(angleRad);

        float worldMoveX = (moveDirection.x * cosAngle) - (moveDirection.z * sinAngle);
        float worldMoveZ = (moveDirection.x * sinAngle) + (moveDirection.z * cosAngle);

        nowPlayerState_.velocity.x = worldMoveX * GameConstants::MOVE_SPEED;
        nowPlayerState_.velocity.z = worldMoveZ * GameConstants::MOVE_SPEED;

        /*
        nowPlayerState_.velocity.x = (moveDirection.x * cosAngle - moveDirection.z * sinAngle) * GameConstants::MOVE_SPEED;
        nowPlayerState_.velocity.z = (moveDirection.x * sinAngle + moveDirection.z * cosAngle) * GameConstants::MOVE_SPEED;*/
    }


    // 2. 处理跳跃
    if (currentInput_.jumpPressed && !nowPlayerState_.isInAir) {
        nowPlayerState_.velocity.y = GameConstants::JUMP_FORCE;
        nowPlayerState_.isInAir = true;
        // std::cout << "[Game] Jump initiated!" << std::endl; // 可以按需保留或移除日志
    }

    // 3. 应用重力
    if (nowPlayerState_.isInAir) {
        nowPlayerState_.velocity.y -= GameConstants::GRAVITY * deltaTime;
        nowPlayerState_.velocity.y = std::max(nowPlayerState_.velocity.y, GameConstants::MAX_FALL_VELOCITY);
    }
    else {
        nowPlayerState_.velocity.y = -0.01f; // 这里是为了让后续碰撞检测不必那么精确
    }

    // 4. 计算无碰撞时的下一位置
    Struct3D potentialNextPos = nowPlayerState_.pos + (nowPlayerState_.velocity * deltaTime);

    // 5. 碰撞检测与处理
    PlayerState nextState = nowPlayerState_;
    nextState.pos = potentialNextPos;
    AABB playerNextAABB = GetPlayerAABB(nextState);
    bool collisionOccurredThisFrame = false;

    // 1）检测与场景中静态障碍物的碰撞 (使用从地图文件加载的障碍物)
    for (const auto& obstacle : currentMap_.obstacles) {
        if (CheckAABBCollision(playerNextAABB, obstacle)) {
            // std::cout << "[Game] Collision detected with obstacle." << std::endl; // 保留
            collisionOccurredThisFrame = true;

            // 简化的碰撞响应：将玩家推回，并停止在该方向上的速度
            // 查找重叠最小的轴，计算各轴上的重叠量
            float overlapX = 0.0f, overlapY = 0.0f, overlapZ = 0.0f;

            // 计算X轴重叠
            if (playerNextAABB.max.x > obstacle.min.x && playerNextAABB.min.x < obstacle.max.x) {
                overlapX = std::min(playerNextAABB.max.x - obstacle.min.x, obstacle.max.x - playerNextAABB.min.x);
            }
            // 计算Y轴重叠
            if (playerNextAABB.max.y > obstacle.min.y && playerNextAABB.min.y < obstacle.max.y) {
                overlapY = std::min(playerNextAABB.max.y - obstacle.min.y, obstacle.max.y - playerNextAABB.min.y);
            }
            // 计算Z轴重叠
            if (playerNextAABB.max.z > obstacle.min.z && playerNextAABB.min.z < obstacle.max.z) {
                overlapZ = std::min(playerNextAABB.max.z - obstacle.min.z, obstacle.max.z - playerNextAABB.min.z);
            }

            // 优先处理Y轴碰撞 (着陆或撞头)
            if (overlapY > 0 && (overlapX == 0 || overlapY <= overlapX) && (overlapZ == 0 || overlapY <= overlapZ)) {
                if (nowPlayerState_.velocity.y <= 0 && playerNextAABB.min.y < obstacle.max.y && nowPlayerState_.pos.y >= obstacle.max.y - GameConstants::PLAYER_HEIGHT * 0.5f) { // 向下运动时撞到顶部
                    potentialNextPos.y = obstacle.max.y + GameConstants::PLAYER_HEIGHT * 0.5f; // 精确放到障碍物顶部再加半高，因为Unity对于scale的定义
                    nowPlayerState_.velocity.y = 0;
                    nowPlayerState_.isInAir = false;
                    // std::cout << "[Game] Landed on obstacle." << std::endl;
                }
                else if (nowPlayerState_.velocity.y > 0 && playerNextAABB.max.y > obstacle.min.y && nowPlayerState_.pos.y <= obstacle.min.y + GameConstants::PLAYER_HEIGHT * 0.5f) { // 向上运动时撞到底部
                    potentialNextPos.y = obstacle.min.y - GameConstants::PLAYER_HEIGHT; // 精确放到障碍物下方
                    nowPlayerState_.velocity.y = 0; // 撞头，停止向上的速度
                    // std::cout << "[Game] Hit obstacle underside." << std::endl;
                }
            }
            // 其次处理X轴碰撞
            else if (overlapX > 0 && (overlapY == 0 || overlapX <= overlapY) && (overlapZ == 0 || overlapX <= overlapZ)) {
                float pushBackEpsilon = 0.001f;
                if (nowPlayerState_.velocity.x > 0 && playerNextAABB.max.x > obstacle.min.x) { //向右
                    potentialNextPos.x = obstacle.min.x - GameConstants::PLAYER_WIDTH * 0.5f - pushBackEpsilon;
                    nowPlayerState_.velocity.x = 0;
                }
                else if (nowPlayerState_.velocity.x < 0 && playerNextAABB.min.x < obstacle.max.x) { //向左
                    potentialNextPos.x = obstacle.max.x + GameConstants::PLAYER_WIDTH * 0.5f + pushBackEpsilon;
                    nowPlayerState_.velocity.x = 0;
                }
                // std::cout << "[Game] Hit obstacle side (X)." << std::endl;
            }
            // 最后处理Z轴碰撞
            else if (overlapZ > 0) {
                float pushBackEpsilon = 0.001f;
                if (nowPlayerState_.velocity.z > 0 && playerNextAABB.max.z > obstacle.min.z) { //向前
                    potentialNextPos.z = obstacle.min.z - GameConstants::PLAYER_DEPTH * 0.5f - pushBackEpsilon;
                    nowPlayerState_.velocity.z = 0;
                }
                else if (nowPlayerState_.velocity.z < 0 && playerNextAABB.min.z < obstacle.max.z) { //向后
                    potentialNextPos.z = obstacle.max.z + GameConstants::PLAYER_DEPTH * 0.5f + pushBackEpsilon;
                    nowPlayerState_.velocity.z = 0;
                }
                // std::cout << "[Game] Hit obstacle side (Z)." << std::endl;
            }
            // 碰撞后重新计算玩家的AABB，因为位置可能已经改变
            nextState.pos = potentialNextPos; // 更新临时状态的位置
            playerNextAABB = GetPlayerAABB(nextState); // 重新获取AABB
        }
    }


    // 2）检测与地面的碰撞
    bool landedOnGroundThisFrame = false;
    float playerBottomY = potentialNextPos.y - GameConstants::PLAYER_HEIGHT * 0.5f;
    if (potentialNextPos.y <= GameConstants::GROUND_LEVEL_Y && nowPlayerState_.velocity.y <= 0) {
        if (!nowPlayerState_.isInAir) { // 如果已经在地面
            potentialNextPos.y = GameConstants::GROUND_LEVEL_Y + GameConstants::PLAYER_HEIGHT * 0.5f;
        }
        else { // 如果刚接触地面
            potentialNextPos.y = GameConstants::GROUND_LEVEL_Y + GameConstants::PLAYER_HEIGHT * 0.5f;
            nowPlayerState_.velocity.y = 0;
            nowPlayerState_.isInAir = false;
            landedOnGroundThisFrame = true;
            // std::cout << "[Game] Landed on ground." << std::endl;
        }
        collisionOccurredThisFrame = true;
    }

    // 6. 更新最终位置
    nowPlayerState_.pos = potentialNextPos;

    // 7. 更新空中状态
    // 如果本帧没有着陆（到地面或障碍物上），并且玩家最终位置高于地面，则玩家在空中
    if (!landedOnGroundThisFrame && !(!nowPlayerState_.isInAir && collisionOccurredThisFrame && nowPlayerState_.velocity.y == 0)) {
        if (nowPlayerState_.pos.y > GameConstants::GROUND_LEVEL_Y) {
            if (!nowPlayerState_.isInAir) { // 之前在地上，现在高于地面 (例如从边缘走下)
                // std::cout << "[Game] Left ground/obstacle." << std::endl;
            }
            nowPlayerState_.isInAir = true;
        }
    }


    // 8. 检查胜利条件
    CheckWinCondition();


    // 9. 重置单帧输入事件
    currentInput_.jumpPressed = false;
}

// 检查玩家是否到达胜利点
void GameHandler::CheckWinCondition() {
    if (!nowPlayerState_.hasWon && currentMap_.loadedSuccessfully) { // 只有在地图加载成功且未胜利时检查
        if (nowPlayerState_.pos.IsCloseTo(currentMap_.victoryPoint, 1.0f)) { // 使用较大的容差判断
            nowPlayerState_.hasWon = true;
            std::cout << "[Game] Player has reached the victory point!" << std::endl;
            // 可以在这里触发一些胜利相关的逻辑，例如停止玩家移动
            nowPlayerState_.velocity = { 0.0f, 0.0f, 0.0f };
        }
    }
}


std::optional<std::string> GameHandler::GetStateDataForNetwork() const {
    game_backend::ServerToClient server_msg;
    game_backend::GameState* state_payload = server_msg.mutable_state(); // 获取GameState负载

    // 填充位置
    game_backend::Vector3* pos_proto = state_payload->mutable_position();
    pos_proto->set_x(nowPlayerState_.pos.x);
    pos_proto->set_y(nowPlayerState_.pos.y);
    pos_proto->set_z(nowPlayerState_.pos.z);

    // 填充速度
    game_backend::Vector3* vel_proto = state_payload->mutable_velocity();
    vel_proto->set_x(nowPlayerState_.velocity.x);
    vel_proto->set_y(nowPlayerState_.velocity.y);
    vel_proto->set_z(nowPlayerState_.velocity.z);

    // 填充旋转
    state_payload->set_y_orientation_angle(nowPlayerState_.yOrientationAngle);

    // 填充其他状态
    state_payload->set_is_in_air(nowPlayerState_.isInAir);
    state_payload->set_has_won(nowPlayerState_.hasWon); // 同步胜利状态

    std::string serialized_data;
    if (!server_msg.SerializeToString(&serialized_data)) {
        std::cerr << "[Game] Error: Failed to serialize game state!" << std::endl;
        return std::nullopt;
    }
    return serialized_data;
}

bool GameHandler::CheckAABBCollision(const AABB& a, const AABB& b) const {
    bool xOverlap = a.max.x > b.min.x && a.min.x < b.max.x;
    bool yOverlap = a.max.y > b.min.y && a.min.y < b.max.y;
    bool zOverlap = a.max.z > b.min.z && a.min.z < b.max.z;
    return xOverlap && yOverlap && zOverlap;
}