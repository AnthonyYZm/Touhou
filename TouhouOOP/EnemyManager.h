#pragma once
#include "Enemy.h"
#include "Bullet.h"
#include <vector>
#include <map>

class EnemyManager {
private:

	std::vector<Enemy*> enemyList;

	unsigned int aliveEnemy;
	unsigned int enemyIdx;
	bool clear;

	std::vector<float> currentPosition;
	int interval;

	DWORD t1, t2;

public:
	EnemyManager();
	~EnemyManager();

	std::map<eType, float> speedMap;

	void createEnemy(const eType& type);
	void move(const eType& type, float speed);
	bool collision(const eType& type, Bullet* bullet);

	void InitRound();
	bool checkEnemyClear();
	void SetStatus(const std::vector<float>& positions, int interval);
	void drawAll(const eType& type); // 新增：负责遍历调用 Enemy->draw()

	std::vector<Enemy*>& getList() { return enemyList; }
	int getAliveEnemy() const { return aliveEnemy; }
	int getEnemyCount() const { return enemyList.size(); }
};