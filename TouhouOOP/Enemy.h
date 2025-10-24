#ifndef ENEMY_H
#define ENEMY_H
#include "Role.h"
#include "Bullet.h"

class Enemy : public Role {

	int type; //round enemy type
	int col;
	int frame;
	int row;
	int enemyWidth;	
	int enemyHeight;
	float enemyX;
	float enemyY;
	int enemyNum;
	int aliveEnemy;
	bool isfire;
	bool clear;
	DWORD t1, t2;
	IMAGE enemy1;	

public:
	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	void draw() override;
	void Init(Enemy* enemy);
	void EnemyX(); //set round's enemyX
	void EnemyNum(); //set round's enemyNum
	void Init();
	void enemyInit();
	void createEnemy(Enemy* enemy);
	void move(float speed, Enemy* enemy);
	bool checkEnemyClear(Enemy* enemy);
	void InitRound();
	void collision(std::vector<Bullet>& bullets, Enemy* enemy);
	
	bool isFire() { return isfire; }
	float getEnemyX() { return enemyX; }
	int getEnemyNum() { return enemyNum; }
	bool isAlive() { return alive; }
	int GetAliveEnemy() { return aliveEnemy; }

};

#endif
