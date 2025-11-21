#pragma once
#include "Role.h"
#include "Bullet.h"

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

	void draw() override;
	void draw2();	
	void EnemyX(); //set round's enemyX
	void EnemyNum(); //set round's enemyNum
	void createEnemy(eType type, Enemy* enemy);
	void move1(float speed, Enemy* enemy);
	void move2(float speed, Enemy* enemy);	
	void move(eType type, Enemy* enemy, float speed);
	bool checkEnemyClear();
	void InitRound();
	void collision(eType type, Bullet* bullet, Enemy* enemy);
	
	bool isFire() const { return isfire; }
	float getEnemyX() const { return enemyX; }
	int getEnemyNum() const { return enemyNum; }
	bool isAlive() const { return alive; }
	int GetAliveEnemy() const { return aliveEnemy; }

};

