#pragma once
#include "Role.h"
#include "Bullet.h"
#include <map>

/// <summary>
/// @brief Enemy class
/// @details Inherited from Role class, represents enemy characters
/// </summary>
class Enemy : public Role {

	unsigned int type; //round enemy type
	int col;
	int frame;
	int row;
	float enemyX;
	float enemyY;
	unsigned int enemyNum; //round's enemyNum
	unsigned int aliveEnemy;
	unsigned int enemyIdx;
	bool isfire;
	bool clear;
	DWORD t1, t2, te1, te2;
	IMAGE enemy1, enemy2;	

public:

	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	static std::map<eType, float> speedMap; 

	void draw() override;
	void draw2();	
	void EnemyX(); //set round's enemyX
	void EnemyNum(); //set round's enemyNum
	void createEnemy(const eType& type, std::vector<Enemy*>& list);
	void move(const eType& type, std::vector<Enemy*>& list, float speed);
	void resetTimers();
	void InitRound();
	bool checkEnemyClear();
	bool collision(const eType& type, Bullet* bullet, std::vector<Enemy*>& list);
	
	bool isFire() const { return isfire; }
	float getEnemyX() const { return enemyX; }
	int getEnemyNum() const { return enemyNum; }
	bool isAlive() const { return alive; }
	int GetAliveEnemy() const { return aliveEnemy; }

	// count: 这一波怪的总数
	// interval: 出怪间隔
	void SetStatus(int count, int interval) {
		enemyNum = count;
	}
};

