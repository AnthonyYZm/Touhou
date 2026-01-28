#pragma once
#include "Enemy.h"
#include "Bullet.h"
#include "Boss.h"       
#include "Movement.h"
#include <vector>
#include <map>
#include <queue>

struct SpawnEvent {
	int startTime;        // 波次开始后的延迟(ms)
	int count;            // 生成数量
	int interval;         // 生成间隔(ms)
	eType type;           // 敌人外观类型
	float startX, startY; // 起始位置
	MoveStrategy moveLogic; // 移动策略
	std::vector<BarrageTask> initTasks; // 初始携带的弹幕任务

	// 如果是 Boss，直接传入 Boss 对象指针 (可选)
	Boss* bossInstance = nullptr;

};

class EnemyManager {
private:

	std::vector<Enemy*> enemyList;

	std::queue<SpawnEvent> eventQueue;
	SpawnEvent currentEvent;
	bool processingEvent;
	int spawnedCount;
	DWORD lastSpawnTime;
	DWORD waveStartTime;

	unsigned int aliveEnemy;
	unsigned int enemyIdx;
	bool clear;

	std::vector<float> currentPosition;
	int interval;

	DWORD t1, t2;

public:
	EnemyManager();
	~EnemyManager();

	void createEnemy(const eType& type);
	void moveEnemy();
	bool collision(const eType& type, Bullet* bullet);

	void InitRound();
	bool checkEnemyClear();
	void SetStatus(const std::vector<float>& positions, int interval);
	void drawAll(); // 新增：负责遍历调用 Enemy->draw()

	std::vector<Enemy*>& getList() { return enemyList; }
	int getAliveEnemy() const { return aliveEnemy; }
	int getEnemyCount() const { return enemyList.size(); }
};