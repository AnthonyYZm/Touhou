#ifndef ENEMYRULE_H
#define ENEMYRULE_H
#include "Enemy.h"
#include "Bullet.h"

class EnemyRule {

	Enemy enemy[15];
	int aliveEnemy;
	bool isfire;
	bool clear;	
	DWORD t1, t2;
	
public:

	EnemyRule();

	void Init();

	int GetAliveEnemy() { return aliveEnemy; }

	void enemyInit();

	void createEnemy();	
	
	void move(float speed);

	bool checkEnemyClear();

	void InitRound();	

	void collision(std::vector<Bullet>& bullets);
	 
	bool isFire() { return isfire; }	

	void EnemyX(); 
	float getEnemyX(); 
	void EnemyNum(); 
	int getEnemyNum();
};

#endif
