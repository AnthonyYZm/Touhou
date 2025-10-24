#ifndef ENEMY_H
#define ENEMY_H
#include "Role.h"

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
	IMAGE enemy1;	
	DWORD t1, t2;

public:
	Enemy(float _x = 0, float _y = 0, int _hp = 1);

	void draw() override;

	void Init();

	void EnemyX(); //set round's enemyX

	float getEnemyX() { return enemyX; }

	void EnemyNum(); //set round's enemyNum

	int getEnemyNum() { return enemyNum; }

	bool isAlive() { return alive; }

};

#endif
