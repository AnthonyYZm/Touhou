#pragma once
#pragma execution_character_set("utf-8")
#include "Enemy.h"
#include "Bullet.h"
#include "Boss.h"       
#include "Movement.h"
#include <cstdint>
#include <vector>

// 关卡作者提供的静态刷怪配置。
// 它只描述“要生成什么”，不保存已生成数量等运行时数据。
struct SpawnGroupDef {
	int count = 0;                       // 本组总敌人数
	int intervalMs = 0;                  // 两只敌人之间的生成间隔，单位毫秒
	int hp = 0;                           // 每只敌人的生命值
	eType type = eType::normal;          // normal 普通敌人，elf 精英敌人
	float startX = 0, startY = 0;        // 每只敌人的出生坐标
	MoveStrategy moveLogic;              // 敌人出生后使用的移动轨迹
	std::vector<BarrageTask> initTasks;  // 每只敌人出生时附带的弹幕任务
};

// 将毫秒换算为 60Hz 游戏逻辑帧，向上取整。
// 例如 500ms 会换算为 30 帧。
constexpr std::uint64_t GameFramesFromMilliseconds(std::uint64_t milliseconds) {
	return (milliseconds * 60 + 999) / 1000;
}

class EnemyManager {
private:
	// 正在逐只生成的刷怪组运行时状态。
	struct PendingSpawnGroup {
		SpawnGroupDef definition;          // 该组的静态配置
		int spawnedCount = 0;              // 已生成数量
		std::uint64_t nextSpawnFrame = 0;   // 下一只应生成的世界帧
		std::uint64_t intervalFrames = 1;   // 生成间隔换算后的帧数
	};

	std::vector<Enemy*> enemyList; // 已创建并由 EnemyManager 负责释放的敌人
	std::vector<PendingSpawnGroup> pendingSpawnGroups; // 尚未生成完毕的刷怪组
	unsigned int aliveEnemy; // 旧统计字段；清场判定使用容器状态，不依赖此计数
	void updateSpawns(std::uint64_t worldFrame);

public:
	EnemyManager();
	~EnemyManager();

	void scheduleSpawnGroup(const SpawnGroupDef& group, std::uint64_t worldFrame);
	void createEnemy(const SpawnGroupDef& group);
	void moveEnemy(std::uint64_t worldFrame);
	void InitRound();
	bool checkEnemyClear();
	void clearEnemy();
	void retreatNonBossEnemies();
	void drawAll(); 
	void outBound();

	std::vector<Enemy*>& getList() { return enemyList; }
	int getAliveEnemy() const { return aliveEnemy; }
	int getEnemyCount() const { return (int)enemyList.size(); }

	void clearList() {
		for (auto enemy : enemyList) {
			delete enemy;
		}
		enemyList.clear();
		pendingSpawnGroups.clear();
	}

	void addEnemy(Enemy* newEnemy) {
		if (newEnemy != nullptr) enemyList.push_back(newEnemy);
	}

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
