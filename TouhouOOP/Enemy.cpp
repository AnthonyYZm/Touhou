#include "Enemy.h"

int const Enemy::normalWidth = 32;
int const Enemy::normalHeight = 27;
int const Enemy::elfWidth = 64;
int const Enemy::elfHeight = 50;

std::map<eType, float> Enemy::speedMap = {
	{eType::normal, 4.5f},
	{eType::elf, 4.5f}
};

Enemy::Enemy(float _x, float _y, int _hp) : Role(_x, _y, _hp) {
	loadimage(&enemy1, L"resource/enemy/enemy.png");
	hp = 1; 
	col = 0; frame = 0; row = 0;
	enemyX = 0; enemyY = 0;
    te1 = 0, te2 = 0;	
	fire = false;
}

void Enemy::EnemyX() { //set round's enemyX

}

void Enemy::draw() {
	te2 = GetTickCount();
	putimagePNG((int)x, (int)y, width, height, &enemy1, sx, sy);
	if (te2 - te1 > 80) {
		row = (row + 1) % frame;
		te1 = te2;
	}
}

void Enemy::move() {
	if (!alive) return;
	x += vx;
	y += vy;
}

void Enemy::draw2() {
	te2 = GetTickCount();
	int sx = row * 64;
	int sy = 454;

	frame = 5;
	putimagePNG((int)x, (int)y, elfWidth, elfHeight, &enemy1, sx, sy);
	if (te2 - te1 > 80) {
		row = (row + 1) % frame;
		te1 = te2;
	}
}






