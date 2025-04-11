// GameHandler.h
#pragma once

#include "3DPos.h"        // ���� Struct3D, PlayerState, PlayerInputState, AABB, Constants �ȶ���
#include "messages.pb.h"  // �������ɵ� Protobuf ��Ϣͷ�ļ�
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>    // ���� std::max ���㷨
#include <optional>     // ���� std::optional (���磬���л�����ʧ��)

// ��Ҫ AsioNetworkManager �������������ʵ�� ProcessReceivedData
// �� AsioNetworkManager Ҳ��Ҫ GameHandler������ѭ��������
// ����������� AsioNetworkManager::ProcessReceivedData ��ʵ�ַ��� GameHandler.h �ײ�
// ��������Ƿ���һ�������� .cpp �ļ��У�����������ͷ�ļ���
// ����Ϊ�˼򻯣����� GameHandler.h �ײ���
#include "AsioNetworkManager.h"


// ���������Ϸ�߼���״̬���º���ײ������
class GameHandler {
public:
    // ���캯��: ��ʼ����Ϸ״̬���ϰ���
    GameHandler() {
        Initialize(); // ���ó�ʼ������
        // ���һЩ��̬�ϰ������ڲ�����ײ
        // ʾ��: һ��λ�� (5, 0.5, 0) ���ģ���СΪ 1x1x1 ���������ϰ���
        obstacles_.push_back(CreateAABB({ 5.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }));
        // ʾ��: һ��λ�� (-2, 1.0, 3) ���ģ���СΪ 1x2x1 �Ľϸ��ϰ���
        obstacles_.push_back(CreateAABB({ -2.0f, 1.0f, 3.0f }, { 1.0f, 2.0f, 1.0f }));
        std::cout << "[Game] GameHandler created and obstacles added." << std::endl;
    }

    // ��ʼ����������Ϸ״̬
    void Initialize() {
        // �����״̬����ΪĬ��ֵ (PlayerState ���캯���ж����)
        nowPlayerState_ = {};
        // ���õ�ǰ����״̬
        currentInput_ = {};
        std::cout << "[Game] Game state initialized." << std::endl;
    }

    // ���������㷴���л�����ġ��ṹ�����������״̬
    void ProcessInput(const PlayerInputState& input) {
        // �����յ�������״̬�洢�������� Update ����ʹ��
        currentInput_ = input;
        // ע�⣺��Ծ (jumpPressed) �ľ���Ч������ Update �����и������״̬�����
    }

    // ���ݵ�ǰ����;�����ʱ�� (deltaTime) ��������Ϸ״̬
    void Update(float deltaTime) {
        // --- 1. �����������Ŀ��ˮƽ�ٶ� ---
        // ������ǰ�Ĵ�ֱ�ٶȣ�ֻ�޸�ˮƽ�ٶȷ���
        Struct3D targetVelocity = { 0.0f, nowPlayerState_.velocity.y, 0.0f };
        // ���������־λ�ۼ�Ŀ���ٶ�
        if (currentInput_.moveForward)  targetVelocity.z += GameConstants::MOVE_SPEED;
        if (currentInput_.moveBackward) targetVelocity.z -= GameConstants::MOVE_SPEED;
        if (currentInput_.moveLeft)   targetVelocity.x -= GameConstants::MOVE_SPEED;
        if (currentInput_.moveRight)  targetVelocity.x += GameConstants::MOVE_SPEED;
        // (δ�����Լ��뱼�ܡ�Ǳ�еȸı��ٶȵ��߼�)

        // �򵥵��ƶ�ģ�ͣ�ֱ�����õ�ǰ�ٶ�ΪĿ���ٶ� (����ʵ��ģ�ͻ�ʹ�ü��ٶ�)
        nowPlayerState_.velocity.x = targetVelocity.x;
        nowPlayerState_.velocity.z = targetVelocity.z;

        // --- 2. ������Ծ ---
        // ֻ�е�����ڵ����� (`!isInAir`) ���ҵ�ǰ֡���յ���Ծָ�� (`jumpPressed`) ʱ��ִ����Ծ
        if (currentInput_.jumpPressed && !nowPlayerState_.isInAir) {
            nowPlayerState_.velocity.y = GameConstants::JUMP_FORCE; // ʩ�����ϵ��ٶȳ���
            nowPlayerState_.isInAir = true; // ����뿪���棬�������״̬
            std::cout << "[Game] Jump initiated!" << std::endl;
        }
        // ע��: currentInput_.jumpPressed Ӧ���ڴ�������� (�� Update ĩβ)����Ϊ����һ����֡�¼�


        // --- 3. Ӧ������ ---
        // �������ڿ��У���Ӧ���������ٶ�
        if (nowPlayerState_.isInAir) {
            nowPlayerState_.velocity.y -= GameConstants::GRAVITY * deltaTime;
            // ����ֱ�����ٶ����������ֵ����ֹ�ٶ���������
            nowPlayerState_.velocity.y = std::max(nowPlayerState_.velocity.y, GameConstants::MAX_FALL_VELOCITY);
        }
        else {
            // ����ڵ����ϣ���ֱ�ٶ�ӦΪ 0 (���߿��Դ���б���߼�)
            // ��ѡ: ����ڵ�����û��ˮƽ���룬����ʩ�ӵ���Ħ��������
            // if (std::abs(targetVelocity.x) < 0.1f && std::abs(targetVelocity.z) < 0.1f) {
            //     nowPlayerState_.velocity.x *= (1.0f - GROUND_FRICTION * deltaTime);
            //     nowPlayerState_.velocity.z *= (1.0f - GROUND_FRICTION * deltaTime);
            // }
            nowPlayerState_.velocity.y = 0; // ȷ���ڵ���ʱ��ֱ�ٶȹ���
        }


        // --- 4. ��������ײʱ��Ǳ����һλ�� ---
        // ��һλ�� = ��ǰλ�� + �ٶ� * ʱ��
        Struct3D potentialNextPos = nowPlayerState_.pos + (nowPlayerState_.velocity * deltaTime);

        // --- 5. ��ײ����봦�� ---

        // ����һ����ʱ״̬�������ڼ��������Ǳ����һλ�õ� AABB
        PlayerState nextState = nowPlayerState_;
        nextState.pos = potentialNextPos;
        AABB playerNextAABB = GetPlayerAABB(nextState); // ��ȡ����ƶ���� AABB

        bool collisionOccurredThisFrame = false; // ��Ǳ�֡�Ƿ����κ���ײ

        // 5a. ����볡���о�̬�ϰ������ײ
        for (const auto& obstacle : obstacles_) {
            // ���� CheckAABBCollision �����ҵ���һ�� AABB �Ƿ����ϰ��� AABB �ص�
            if (CheckAABBCollision(playerNextAABB, obstacle)) {
                std::cout << "[Game] Collision detected with obstacle at min{"
                    << obstacle.min.x << "," << obstacle.min.y << "," << obstacle.min.z << "} max{"
                    << obstacle.max.x << "," << obstacle.max.y << "," << obstacle.max.z << "}" << std::endl;
                collisionOccurredThisFrame = true;

                // --- !!! �����������ײ��Ӧ - ������ʾ !!! ---
                // Ŀ��: ��ֹ����ƶ����ϰ��
                // ��������ײ��Ӧ��Ҫ���㴩͸��Ⱥͷ���Ȼ������ƿ���
                // �����ʵ�ַǳ��򻯣�Ч�����ܲ����룬��Ҫ�����Ľ���

                // �����ص���С���� (��Ϊ���Ƶ��Ƴ�����) - ����һ�ֳ����ĵ��������ļ򻯷���
                // ��������ϵ��ص��� (����ص�)
                float overlapX = 0.0f, overlapY = 0.0f, overlapZ = 0.0f;
                if (playerNextAABB.max.x > obstacle.min.x && playerNextAABB.min.x < obstacle.max.x)
                    overlapX = std::min(playerNextAABB.max.x - obstacle.min.x, obstacle.max.x - playerNextAABB.min.x);
                if (playerNextAABB.max.y > obstacle.min.y && playerNextAABB.min.y < obstacle.max.y)
                    overlapY = std::min(playerNextAABB.max.y - obstacle.min.y, obstacle.max.y - playerNextAABB.min.y);
                if (playerNextAABB.max.z > obstacle.min.z && playerNextAABB.min.z < obstacle.max.z)
                    overlapZ = std::min(playerNextAABB.max.z - obstacle.min.z, obstacle.max.z - playerNextAABB.min.z);

                // ���ȴ��� Y ����ײ (���磬�������嶥����ײ������ײ�)
                if (overlapY > 0 && overlapY <= overlapX && overlapY <= overlapZ) {
                    if (nowPlayerState_.velocity.y <= 0 && playerNextAABB.min.y < obstacle.max.y) { // �����˶�ʱײ������ -> ��������
                        potentialNextPos.y = obstacle.max.y; // ����� Y ���꾫ȷ�ŵ��ϰ��ﶥ��
                        nowPlayerState_.velocity.y = 0;      // ֹͣ��ֱ�ٶ�
                        nowPlayerState_.isInAir = false;     // �������վ���ϰ�����
                        std::cout << "[Game] Landed on obstacle." << std::endl;
                    }
                    else if (nowPlayerState_.velocity.y > 0 && playerNextAABB.max.y > obstacle.min.y) { // �����˶�ʱײ���ײ�
                        potentialNextPos.y = obstacle.min.y - GameConstants::PLAYER_HEIGHT; // �ŵ��ϰ����·�
                        nowPlayerState_.velocity.y = 0; // ֹͣ���ϵ��ٶ� (ײͷ)
                        std::cout << "[Game] Hit obstacle underside." << std::endl;
                    }
                }
                // ��δ��� X ����ײ (������ײ)
                else if (overlapX > 0 && overlapX <= overlapY && overlapX <= overlapZ) {
                    float pushBackEpsilon = 0.001f; // ��һ��΢С��ƫ�Ʒ�ֹ��ס
                    if (nowPlayerState_.velocity.x > 0 && playerNextAABB.max.x > obstacle.min.x) { // �����˶�ײ�������
                        potentialNextPos.x = obstacle.min.x - GameConstants::PLAYER_WIDTH * 0.5f - pushBackEpsilon; // �Ƶ��ϰ������
                        nowPlayerState_.velocity.x = 0; // ֹͣ X ���ٶ�
                    }
                    else if (nowPlayerState_.velocity.x < 0 && playerNextAABB.min.x < obstacle.max.x) { // �����˶�ײ���Ҳ���
                        potentialNextPos.x = obstacle.max.x + GameConstants::PLAYER_WIDTH * 0.5f + pushBackEpsilon; // �Ƶ��ϰ����Ҳ�
                        nowPlayerState_.velocity.x = 0; // ֹͣ X ���ٶ�
                    }
                    std::cout << "[Game] Hit obstacle side (X)." << std::endl;
                }
                // ����� Z ����ײ (ǰ����ײ)
                else if (overlapZ > 0) {
                    float pushBackEpsilon = 0.001f;
                    if (nowPlayerState_.velocity.z > 0 && playerNextAABB.max.z > obstacle.min.z) { // ��ǰ�˶�ײ�������
                        potentialNextPos.z = obstacle.min.z - GameConstants::PLAYER_DEPTH * 0.5f - pushBackEpsilon; // �Ƶ��ϰ������
                        nowPlayerState_.velocity.z = 0; // ֹͣ Z ���ٶ�
                    }
                    else if (nowPlayerState_.velocity.z < 0 && playerNextAABB.min.z < obstacle.max.z) { // ����˶�ײ��ǰ����
                        potentialNextPos.z = obstacle.max.z + GameConstants::PLAYER_DEPTH * 0.5f + pushBackEpsilon; // �Ƶ��ϰ���ǰ��
                        nowPlayerState_.velocity.z = 0; // ֹͣ Z ���ٶ�
                    }
                    std::cout << "[Game] Hit obstacle side (Z)." << std::endl;
                }

                // ��Ҫ: �ڴ�����һ����ײ�����������Ӧ�����¼��� AABB ���ٴμ���������������ײ��
                // ��ǰ��ʵ���ǻ��ڳ�ʼ potentialNextPos �������ϰ�����м��͵�����
                // ����ܵ����ڸ�������£�����䣩��Ӧ����ȷ��
            }
        }


        // 5b. �����������ײ
        // (δ�����԰ѵ���Ҳ��Ϊһ���޴�� AABB ��ͳһ����)
        // ��ǰ�߼���������Ǳ��λ�õ��ڻ���ڵ��� Y ���꣬�������������ֹ
        bool landedOnGroundThisFrame = false; // ��Ǳ�֡�Ƿ���½
        if (potentialNextPos.y <= GameConstants::GROUND_LEVEL_Y && nowPlayerState_.velocity.y <= 0) {
            // �������Ѿ��ڵ����ϣ�ֻ��ȷ�� Y ���겻���ڵ���
            if (!nowPlayerState_.isInAir) {
                potentialNextPos.y = GameConstants::GROUND_LEVEL_Y;
            }
            // �����ҸսӴ�����
            else {
                potentialNextPos.y = GameConstants::GROUND_LEVEL_Y; // �� Y ���꾫ȷ���õ�����
                nowPlayerState_.velocity.y = 0;      // ֹͣ��ֱ�����ٶ�
                nowPlayerState_.isInAir = false;    // ���Ϊ���ڿ��� (����½)
                landedOnGroundThisFrame = true;     // ��Ǳ�֡��������½
                std::cout << "[Game] Landed on ground." << std::endl;
            }
            collisionOccurredThisFrame = true; // ���Ӵ�����Ҳ��Ϊһ����ײ
        }

        // --- 6. ��������λ�� ---
        // Ӧ�þ�����ײ������Ӧ�������λ��
        nowPlayerState_.pos = potentialNextPos;

        // --- 7. ���¿���״̬ ---
        // ����ڱ�֡����ײ������û��ȷ�������½ (���ϰ���������)��
        // ������ҵ�����λ�ø��ڵ��棬��ô���Ӧ�ô��ڿ���״̬��
        if (!landedOnGroundThisFrame && !(!nowPlayerState_.isInAir && collisionOccurredThisFrame)) { // ���û��½���Ҳ��Ǳ�������ײǿ�����ڵ���
            if (nowPlayerState_.pos.y > GameConstants::GROUND_LEVEL_Y) {
                // ���֮ǰ�ڵ��ϵ�����λ�ø��ڵ��� (����ӱ�Ե����)
                if (!nowPlayerState_.isInAir) {
                    std::cout << "[Game] Left ground." << std::endl;
                }
                nowPlayerState_.isInAir = true; // ���Ϊ�ڿ���
            }
            // else (���λ�õ��ڵ���Y��û���ٶȣ�Ӧ���Ѿ������汻��Ϊ false ��)
        }


        // --- 8. ���õ�֡�����¼� ---
        // ��Ծָ��ֻ�ڰ��µ���һ֡��Ч�����������Ҫ����
        currentInput_.jumpPressed = false;

    }

    // ����ǰ��Ϸ״̬���л�Ϊ Protobuf �������ַ������������緢��
    // ������л�ʧ�ܣ����� std::nullopt
    std::optional<std::string> GetStateDataForNetwork() const {
        // 1. ����������Ϣ���� ServerToClient
        game_backend::ServerToClient server_msg;
        // 2. ��ȡ������Ϣ������Ϊ GameState �Ŀɱ为�ز���
        //    (��Ϊ������ .proto ������ oneof payload { GameState state = 1; })
        game_backend::GameState* state_payload = server_msg.mutable_state();

        // --- ���л�ʹ�õ� (��� Protobuf ����) ---
        // 3. ��� GameState �����е�����
        // 3a. ���λ����Ϣ
        game_backend::Vector3* pos_proto = state_payload->mutable_position(); // ��ȡλ���ֶεĿɱ�ָ��
        pos_proto->set_x(nowPlayerState_.pos.x);
        pos_proto->set_y(nowPlayerState_.pos.y);
        pos_proto->set_z(nowPlayerState_.pos.z);

        // 3b. ����ٶ���Ϣ
        game_backend::Vector3* vel_proto = state_payload->mutable_velocity(); // ��ȡ�ٶ��ֶεĿɱ�ָ��
        vel_proto->set_x(nowPlayerState_.velocity.x);
        vel_proto->set_y(nowPlayerState_.velocity.y);
        vel_proto->set_z(nowPlayerState_.velocity.z);

        // 3c. �������״̬��Ϣ
        state_payload->set_is_in_air(nowPlayerState_.isInAir);
        // (δ�����������������Ҫͬ����״̬...)

        // 4. �����õĶ�����Ϣ���� (server_msg) ���л�Ϊ�������ַ���
        std::string serialized_data;
        if (!server_msg.SerializeToString(&serialized_data)) {
            // ������л�ʧ��
            std::cerr << "[Game] Error: Failed to serialize game state!" << std::endl;
            return std::nullopt; // ���ؿյ� optional ��ʾʧ��
        }

        // 5. ���ذ������л�����������ݵ��ַ���
        return serialized_data;
    }

private:
    // ������� AABB �Ƿ�����ײ (�ص�)
    // const ��Ա��������Ϊ�����޸� GameHandler ��״̬
    bool CheckAABBCollision(const AABB& a, const AABB& b) const {
        // �������������϶������ص������㷢����ײ
        // ��� X ���Ƿ����ص� (a ���ұ߽� > b ����߽� AND a ����߽� < b ���ұ߽�)
        bool xOverlap = a.max.x > b.min.x && a.min.x < b.max.x;
        // ��� Y ���Ƿ����ص�
        bool yOverlap = a.max.y > b.min.y && a.min.y < b.max.y;
        // ��� Z ���Ƿ����ص�
        bool zOverlap = a.max.z > b.min.z && a.min.z < b.max.z;

        // ֻ�е������ᶼ�ص�ʱ�����ж�Ϊ��ײ
        return xOverlap && yOverlap && zOverlap;
    }


    // �洢��ǰ�����״̬
    PlayerState nowPlayerState_;
    // �洢��ǰ֡��������������ͼ
    PlayerInputState currentInput_;
    // �洢�ؿ��еľ�̬�ϰ��� (AABB �б�)
    std::vector<AABB> obstacles_;

    // --- �����Ǳ��Ƴ�������ľɺ��� ---
    // TransKeyString (�� Protobuf �����л����)
    // GetStateString (�� GetStateDataForNetwork ���)
    // ProcessMessage (�� ProcessInput �� Protobuf �����л����)
};
