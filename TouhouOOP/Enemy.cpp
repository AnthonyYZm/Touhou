#include "Enemy.h"

/*
	Normal enemies
	informations:
		- Random number of enemies (1~5)
		- Random x position (left 1/4 or right 1/4 of the screen)
		- Move
		- Draw
		- 1 HP
*/

/*
	???Combine barragies functions with enemy class later
*/

Enemy::Enemy(float _x, float _y, int _hp) : Role(_x, _y, _hp) {
	loadimage(&enemy1, L"resource/enemy/enemy.png");
	type = 0;
	hp = 1; 
	col = 0; frame = 0; row = 0;
	enemyWidth = 32.5; enemyHeight = 27;
	enemyX = 0; enemyY = 0;
	enemyNum = 0;	
	t1 = GetTickCount(); t2 = 0;	
}

void Enemy::EnemyX() { //set round's enemyX
	srand(GetTickCount());
	int RL = rand() % 2;
	enemyX = (RL == 0) ? (rand() % (WIDTH / 4 + 32)) : (rand() % (WIDTH / 4) + WIDTH * 3 / 4 + 32);
}

void Enemy::EnemyNum() { //set round's enemyNum
	srand(GetTickCount());
	enemyNum = rand() % 5 + 1;
}

void Enemy::Init() {
	x = enemyX;
	y = 0;
	hp = 1;
	alive = false;
}

void Enemy::draw() {
	t2 = GetTickCount();
	int sx = row * enemyWidth;
	int sy = 322;
	frame = 4;
	putimagePNG(x, y, enemyWidth, enemyHeight, &enemy1, sx, sy);
	if (t2 - t1 > 80) {
		row = (row + 1) % frame;
		t1 = t2;
	}
}





