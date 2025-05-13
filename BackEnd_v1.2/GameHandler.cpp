// GameHandler.cpp

#include "GameHandler.h"
#include "AsioNetworkManager.h"
// AsioNetworkManager.h ������ GameHandler.h �У����ﲻ��Ҫ�ظ�������
// ����� GameHandler.cpp ��ֱ��ʹ���� AsioNetworkManager �ľ����Ա���������Ҫ
// #include "AsioNetworkManager.h" // ȷ�� AsioNetworkManager ����������ɼ�

// GameHandler ���캯��
GameHandler::GameHandler() {
    Initialize(); // ���ó�ʼ���������ú��������ص�ͼ
}

// ��ʼ����Ϸ״̬���������ļ����ص�ͼ����
void GameHandler::Initialize() {
    nowPlayerState_ = {}; // �������״̬
    currentInput_ = {};   // ��������״̬

    // ���ص�ͼ����
    currentMap_ = LoadMapFromFile(GameConstants::DEFAULT_MAP_FILE);
    if (!currentMap_.loadedSuccessfully) {
        std::cerr << "[Game] Error: Failed to load map data from " << GameConstants::DEFAULT_MAP_FILE
            << ". Using empty map." << std::endl;
        // ����ѡ�����һ��Ĭ�ϵĿյ�ͼ�����׳��쳣
        currentMap_.obstacles.clear();
        currentMap_.victoryPoint = { 0.0f, 0.0f, 10.0f }; // ����һ��Ĭ��ʤ�����Է���һ
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


// ���ļ����ص�ͼ����
// �ļ���ʽ:
// ��һ��: victory_point x y z
// ֮��ÿ��: obstacle_aabb min_x min_y min_z max_x max_y max_z
MapData GameHandler::LoadMapFromFile(const std::string& filename) {
    MapData mapData;
    mapData.loadedSuccessfully = false; // Ĭ�ϼ���ʧ��
    std::ifstream mapFile(filename);
    std::string line;

    if (!mapFile.is_open()) {
        std::cerr << "[Game] Error: Could not open map file: " << filename << std::endl;
        return mapData; // ���ؼ���ʧ�ܵ�MapData
    }

    // ��ȡʤ����
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
            return mapData; // ���ؼ���ʧ�ܵ�MapData
        }
    }
    else {
        std::cerr << "[Game] Error: Map file is empty or could not read victory point." << std::endl;
        mapFile.close();
        return mapData; // ���ؼ���ʧ�ܵ�MapData
    }

    // ��ȡ�ϰ���
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
        else if (!line.empty() && line[0] != '#') { // ���Կ��к�ע����
            std::cerr << "[Game] Warning: Skipping invalid line in map file: " << line << std::endl;
        }
    }

    mapFile.close();
    mapData.loadedSuccessfully = true; // ��Ǽ��سɹ�
    return mapData;
}


void GameHandler::ProcessInput(const PlayerInputState& input) {
    currentInput_ = input;
}

void GameHandler::Update(float deltaTime) {
    // �������Ѿ�ʤ�����򲻸�����Ϸ�߼�
    if (nowPlayerState_.hasWon) {
        // ����ѡ��������Ծ���룬��ֹ��ʤ�������յ���Ծָ��Ӱ��״̬
        currentInput_.jumpPressed = false;
        return;
    }

    // 1. �����������Ŀ��ˮƽ�ٶ�
    Struct3D targetVelocity = { 0.0f, nowPlayerState_.velocity.y, 0.0f };
    if (currentInput_.moveForward)  targetVelocity.z += GameConstants::MOVE_SPEED;
    if (currentInput_.moveBackward) targetVelocity.z -= GameConstants::MOVE_SPEED;
    if (currentInput_.moveLeft)   targetVelocity.x -= GameConstants::MOVE_SPEED;
    if (currentInput_.moveRight)  targetVelocity.x += GameConstants::MOVE_SPEED;

    nowPlayerState_.velocity.x = targetVelocity.x;
    nowPlayerState_.velocity.z = targetVelocity.z;

    // 2. ������Ծ
    if (currentInput_.jumpPressed && !nowPlayerState_.isInAir) {
        nowPlayerState_.velocity.y = GameConstants::JUMP_FORCE;
        nowPlayerState_.isInAir = true;
        // std::cout << "[Game] Jump initiated!" << std::endl; // ���԰��豣�����Ƴ���־
    }

    // 3. Ӧ������
    if (nowPlayerState_.isInAir) {
        nowPlayerState_.velocity.y -= GameConstants::GRAVITY * deltaTime;
        nowPlayerState_.velocity.y = std::max(nowPlayerState_.velocity.y, GameConstants::MAX_FALL_VELOCITY);
    }
    else {
        nowPlayerState_.velocity.y = 0;
    }

    // 4. ��������ײʱ����һλ��
    Struct3D potentialNextPos = nowPlayerState_.pos + (nowPlayerState_.velocity * deltaTime);

    // 5. ��ײ����봦��
    PlayerState nextState = nowPlayerState_;
    nextState.pos = potentialNextPos;
    AABB playerNextAABB = GetPlayerAABB(nextState);
    bool collisionOccurredThisFrame = false;

    // 1������볡���о�̬�ϰ������ײ (ʹ�ôӵ�ͼ�ļ����ص��ϰ���)
    for (const auto& obstacle : currentMap_.obstacles) {
        if (CheckAABBCollision(playerNextAABB, obstacle)) {
            // std::cout << "[Game] Collision detected with obstacle." << std::endl; // ���豣��
            collisionOccurredThisFrame = true;

            // �򻯵���ײ��Ӧ��������ƻأ���ֹͣ�ڸ÷����ϵ��ٶ�
            // �����ص���С���ᣬ��������ϵ��ص���
            float overlapX = 0.0f, overlapY = 0.0f, overlapZ = 0.0f;

            // ����X���ص�
            if (playerNextAABB.max.x > obstacle.min.x && playerNextAABB.min.x < obstacle.max.x) {
                overlapX = std::min(playerNextAABB.max.x - obstacle.min.x, obstacle.max.x - playerNextAABB.min.x);
            }
            // ����Y���ص�
            if (playerNextAABB.max.y > obstacle.min.y && playerNextAABB.min.y < obstacle.max.y) {
                overlapY = std::min(playerNextAABB.max.y - obstacle.min.y, obstacle.max.y - playerNextAABB.min.y);
            }
            // ����Z���ص�
            if (playerNextAABB.max.z > obstacle.min.z && playerNextAABB.min.z < obstacle.max.z) {
                overlapZ = std::min(playerNextAABB.max.z - obstacle.min.z, obstacle.max.z - playerNextAABB.min.z);
            }

            // ���ȴ���Y����ײ (��½��ײͷ)
            if (overlapY > 0 && (overlapX == 0 || overlapY <= overlapX) && (overlapZ == 0 || overlapY <= overlapZ)) {
                if (nowPlayerState_.velocity.y <= 0 && playerNextAABB.min.y < obstacle.max.y && nowPlayerState_.pos.y >= obstacle.max.y - GameConstants::PLAYER_HEIGHT * 0.5f) { // �����˶�ʱײ������
                    potentialNextPos.y = obstacle.max.y; // ��ȷ�ŵ��ϰ��ﶥ��
                    nowPlayerState_.velocity.y = 0;
                    nowPlayerState_.isInAir = false;
                    // std::cout << "[Game] Landed on obstacle." << std::endl;
                }
                else if (nowPlayerState_.velocity.y > 0 && playerNextAABB.max.y > obstacle.min.y && nowPlayerState_.pos.y <= obstacle.min.y + GameConstants::PLAYER_HEIGHT * 0.5f) { // �����˶�ʱײ���ײ�
                    potentialNextPos.y = obstacle.min.y - GameConstants::PLAYER_HEIGHT; // ��ȷ�ŵ��ϰ����·�
                    nowPlayerState_.velocity.y = 0; // ײͷ��ֹͣ���ϵ��ٶ�
                    // std::cout << "[Game] Hit obstacle underside." << std::endl;
                }
            }
            // ��δ���X����ײ
            else if (overlapX > 0 && (overlapY == 0 || overlapX <= overlapY) && (overlapZ == 0 || overlapX <= overlapZ)) {
                float pushBackEpsilon = 0.001f;
                if (nowPlayerState_.velocity.x > 0 && playerNextAABB.max.x > obstacle.min.x) { //����
                    potentialNextPos.x = obstacle.min.x - GameConstants::PLAYER_WIDTH * 0.5f - pushBackEpsilon;
                    nowPlayerState_.velocity.x = 0;
                }
                else if (nowPlayerState_.velocity.x < 0 && playerNextAABB.min.x < obstacle.max.x) { //����
                    potentialNextPos.x = obstacle.max.x + GameConstants::PLAYER_WIDTH * 0.5f + pushBackEpsilon;
                    nowPlayerState_.velocity.x = 0;
                }
                // std::cout << "[Game] Hit obstacle side (X)." << std::endl;
            }
            // �����Z����ײ
            else if (overlapZ > 0) {
                float pushBackEpsilon = 0.001f;
                if (nowPlayerState_.velocity.z > 0 && playerNextAABB.max.z > obstacle.min.z) { //��ǰ
                    potentialNextPos.z = obstacle.min.z - GameConstants::PLAYER_DEPTH * 0.5f - pushBackEpsilon;
                    nowPlayerState_.velocity.z = 0;
                }
                else if (nowPlayerState_.velocity.z < 0 && playerNextAABB.min.z < obstacle.max.z) { //���
                    potentialNextPos.z = obstacle.max.z + GameConstants::PLAYER_DEPTH * 0.5f + pushBackEpsilon;
                    nowPlayerState_.velocity.z = 0;
                }
                // std::cout << "[Game] Hit obstacle side (Z)." << std::endl;
            }
            // ��ײ�����¼�����ҵ�AABB����Ϊλ�ÿ����Ѿ��ı�
            nextState.pos = potentialNextPos; // ������ʱ״̬��λ��
            playerNextAABB = GetPlayerAABB(nextState); // ���»�ȡAABB
        }
    }


    // 2�������������ײ
    bool landedOnGroundThisFrame = false;
    if (potentialNextPos.y <= GameConstants::GROUND_LEVEL_Y && nowPlayerState_.velocity.y <= 0) {
        if (!nowPlayerState_.isInAir) { // ����Ѿ��ڵ���
            potentialNextPos.y = GameConstants::GROUND_LEVEL_Y;
        }
        else { // ����սӴ�����
            potentialNextPos.y = GameConstants::GROUND_LEVEL_Y;
            nowPlayerState_.velocity.y = 0;
            nowPlayerState_.isInAir = false;
            landedOnGroundThisFrame = true;
            // std::cout << "[Game] Landed on ground." << std::endl;
        }
        collisionOccurredThisFrame = true;
    }

    // 6. ��������λ��
    nowPlayerState_.pos = potentialNextPos;

    // 7. ���¿���״̬
    // �����֡û����½����������ϰ����ϣ��������������λ�ø��ڵ��棬������ڿ���
    if (!landedOnGroundThisFrame && !(!nowPlayerState_.isInAir && collisionOccurredThisFrame && nowPlayerState_.velocity.y == 0)) {
        if (nowPlayerState_.pos.y > GameConstants::GROUND_LEVEL_Y) {
            if (!nowPlayerState_.isInAir) { // ֮ǰ�ڵ��ϣ����ڸ��ڵ��� (����ӱ�Ե����)
                // std::cout << "[Game] Left ground/obstacle." << std::endl;
            }
            nowPlayerState_.isInAir = true;
        }
    }


    // 8. ���ʤ������
    CheckWinCondition();


    // 9. ���õ�֡�����¼�
    currentInput_.jumpPressed = false;
}

// �������Ƿ񵽴�ʤ����
void GameHandler::CheckWinCondition() {
    if (!nowPlayerState_.hasWon && currentMap_.loadedSuccessfully) { // ֻ���ڵ�ͼ���سɹ���δʤ��ʱ���
        if (nowPlayerState_.pos.IsCloseTo(currentMap_.victoryPoint, 1.0f)) { // ʹ�ýϴ���ݲ��ж�
            nowPlayerState_.hasWon = true;
            std::cout << "[Game] Player has reached the victory point!" << std::endl;
            // ���������ﴥ��һЩʤ����ص��߼�������ֹͣ����ƶ�
            nowPlayerState_.velocity = { 0.0f, 0.0f, 0.0f };
        }
    }
}


std::optional<std::string> GameHandler::GetStateDataForNetwork() const {
    game_backend::ServerToClient server_msg;
    game_backend::GameState* state_payload = server_msg.mutable_state(); // ��ȡGameState����

    // ���λ��
    game_backend::Vector3* pos_proto = state_payload->mutable_position();
    pos_proto->set_x(nowPlayerState_.pos.x);
    pos_proto->set_y(nowPlayerState_.pos.y);
    pos_proto->set_z(nowPlayerState_.pos.z);

    // ����ٶ�
    game_backend::Vector3* vel_proto = state_payload->mutable_velocity();
    vel_proto->set_x(nowPlayerState_.velocity.x);
    vel_proto->set_y(nowPlayerState_.velocity.y);
    vel_proto->set_z(nowPlayerState_.velocity.z);

    // �������״̬
    state_payload->set_is_in_air(nowPlayerState_.isInAir);
    state_payload->set_has_won(nowPlayerState_.hasWon); // ͬ��ʤ��״̬

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