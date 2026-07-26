#pragma once
#include "Hero.h"
#include "EnemyManager.h"
#include "Barrage.h"
#include "RLstate.h"
#include <vector>

class RLEnvironment {
private:
	Hero* heroRef;               // 引用游戏中的自机
	EnemyManager* enemiesRef;    // 引用游戏中的敌人管理器
	std::vector<Barrage*>* barrListRef; // 引用弹幕列表

	int framesSurvived;          // 当前回合存活帧数
	int currentLevel;            // 课程学习的难度级别

	// 内部工具函数
	void SortBulletsByDistance(std::vector<Barrage*>& sortedList);
	void ClearBullets();
	Enemy* GetTrainingBoss();
	void SpawnTrainingBullet(float x, float y, float vx, float vy);
	void UpdateTrainingBullets();
	bool CheckHeroHit(const Barrage& bullet) const;

public:
	RLEnvironment(Hero* h, EnemyManager* e, std::vector<Barrage*>* b);
	~RLEnvironment() = default;

	// === 对应 Gym API 的三大核心函数 ===

	// 1. 重置环境 (Reset)
	void Reset(int difficultyLevel);

	// 2. 执行动作并步进一帧 (Step)
	// action: 比如 0=不动, 1=上, 2=下, 3=左, 4=右
	void Step(int action);

	// 3. 提取当前观察状态 (Observation)
	RLState GetObservation();

	// 4. 计算这一帧的奖励 (Reward)
	float CalculateReward();

	// 5. 检查是否结束 (Done)
	bool IsDone();
};
