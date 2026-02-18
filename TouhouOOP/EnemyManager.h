#pragma once
#include "Enemy.h"
#include "Bullet.h"
#include "Boss.h"       
#include "Movement.h"
#include <vector>
#include <map>
#include <queue>
#include <list>

struct SpawnEvent {
	int startTime = 0;        // 波次开始后的延迟(ms)
	int count = 0;            // 生成数量
	int interval = 0;         // 生成间隔(ms)
	int hp = 0;               // 生命值
	eType type;           // 敌人外观类型
	float startX = 0, startY = 0; // 起始位置
	MoveStrategy moveLogic; // 移动策略
	std::vector<BarrageTask> initTasks; // 初始携带的弹幕任务

	Boss* bossInstance = nullptr;
	int spawnedCount = 0; 
	DWORD lastSpawnTime = 0;

};

struct waveData {
	int waveDelay = 0; // 在开始这一波之前，需要等待多久 (ms)
	std::vector<SpawnEvent> events;
};

class EnemyManager {
private:

	std::vector<Enemy*> enemyList;
	std::queue<SpawnEvent> eventQueue;
	std::list<SpawnEvent> activeEvents;
	DWORD waveStartTime = 0;

	unsigned int aliveEnemy;
	unsigned int enemyIdx;
	bool clear;

	std::vector<float> currentPosition;
	int interval = 0;

	DWORD t1, t2;

public:
	EnemyManager();
	~EnemyManager();

	void updateSpawns();
	void createEnemy(const SpawnEvent& ev);
	void moveEnemy();
	void InitRound();
	bool checkEnemyClear();
	void clearEnemy();
	void setWave(const std::vector<SpawnEvent>& event);
	void drawAll(); 
	void outBound();

	std::vector<Enemy*>& getList() { return enemyList; }
	int getAliveEnemy() const { return aliveEnemy; }
	int getEnemyCount() const { return (int)enemyList.size(); }

	struct DropReq {
		float x, y;
		int count; 
	};
	std::vector<DropReq> dropQueue;
	std::vector<DropReq> popDrops() {
		std::vector<DropReq> res = dropQueue;
		dropQueue.clear();
		return res;
	}
};