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
	enemyX = 0; enemyY = 0;
	enemyNum = 0;	
	t1 = 0; t2 = 0, te1 = 0, te2 = 0;	
	aliveEnemy = 0;	
	enemyIdx = 0;	
	clear = true;
}

void Enemy::EnemyX() { //set round's enemyX
	srand(GetTickCount());
	int RL = rand() % 2;
	float float_width = static_cast<float>(WIDTH);
	if (RL == 0) {
		enemyX = (float)rand() / (float)RAND_MAX * (float_width / 4.0f + 32.0f);
	}
	else {
		enemyX = (float)rand() / (float)RAND_MAX * (float_width / 4.0f) + (float_width * 3.0f / 4.0f + 32.0f);
	}
}

void Enemy::EnemyNum() { //set round's enemyNum
	srand(GetTickCount());
	enemyNum = rand() % 5 + 1;
}

void Enemy::draw() {
	te2 = GetTickCount();
	int sx = row * 32.5f;
	int sy = 322;
	frame = 4;
	putimagePNG(x, y, 32.5f, 27.0f, &enemy1, sx, sy);
	if (te2 - te1 > 80) {
		row = (row + 1) % frame;
		te1 = te2;
	}
}

void Enemy::draw2() {
	te2 = GetTickCount();
	int sx = row * 64;
	int sy = 454;
	frame = 5;
	putimagePNG(x, y, 64, 50, &enemy1, sx, sy);
	if (te2 - te1 > 80) {
		row = (row + 1) % frame;
		te1 = te2;
	}
}

void Enemy::createEnemy(eType type, Enemy* enemy) {
	t2 = GetTickCount();
	switch (type) {
		case eType::normal:
			/*Creat normal enemy*/
		{
			if (t2 - t1 >= 1000 && enemyIdx < getEnemyNum()) {
				enemy[enemyIdx].x = getEnemyX();
				enemy[enemyIdx].y = 0;
				enemy[enemyIdx].alive = true;
				enemy[enemyIdx].hp = 1;
				enemyIdx++;
				aliveEnemy++;	
				clear = false;	
				t1 = t2;
			}
			for (int i = 0; i < enemyIdx; i++) {
				if (enemy[i].alive) enemy[i].draw();
			}
			break;
		}
		case eType::elf:
			/*Boss 1*/
		{
			if (clear) {
				enemy->x = WIDTH * 0.5 + LeftEdge;
				enemy->y = TopEdge;
				enemy->alive = true;
				enemy->hp = 10;
				aliveEnemy = 1;
				clear = false;
			}
			if (enemy->alive) {
				enemy->draw2();
			}
			break;
		}
		default:
			break;
	}
}

void Enemy::move1(float speed, Enemy* enemy) {
	for (int i = 0; i < enemyIdx; i++) {
		if (enemy[i].y <= HEIGHT / 3 + 16) {
			enemy[i].y += speed;
			(getEnemyX() <= WIDTH / 2 + 32) ? enemy[i].x += (speed * 0.5) : enemy[i].x -= (speed * 0.5);
		}
		if (getEnemyX() <= WIDTH / 2 + 32 && enemy[i].y > HEIGHT / 3 + 16) {
			if (enemy[0].x <= WIDTH / 3 * 2 + 32 && i == 0) {
				enemy[0].x += speed;
			}
			if (i > 0 && (enemy[i].x <= enemy[i - 1].x - 50)) {
				enemy[i].x += speed;
			}
		}
		else if (getEnemyX() > WIDTH / 2 + 32 && enemy[i].y > HEIGHT / 3 + 16) {
			if (enemy[0].x >= WIDTH / 3 + 32 && i == 0) {
				enemy[0].x -= speed;
			}
			if (i > 0 && (enemy[i].x >= enemy[i - 1].x + 50)) {
				enemy[i].x -= speed;
			}
		}
	}
}

void Enemy::move2(float speed, Enemy* enemy) {
	//Boss 1 movement
	if (enemy->y <= CentralY) {
		enemy->y += speed;
	}
}

void Enemy::move(eType type, Enemy* enemy, float speed) {
	switch (type)
	{
	case eType::normal:
		move1(speed, enemy);
		break;	
	case eType::elf:
		move2(speed, enemy);
		break;
	default:
		break;
	}
}

bool Enemy::checkEnemyClear() {
	if (aliveEnemy == 0) return true;
	else return false;
}

void Enemy::collision(eType type, Bullet* bullet, Enemy* enemy) {
	for (auto& b : bullet->bulletList) {
		if (b.alive) {
			switch (type)
			{
			case eType::normal:
			{
				for (int j = 0; j < enemyIdx; j++) {
					if (enemy[j].alive && b.x >= enemy[j].x && b.x <= enemy[j].x + 32 &&
						b.y >= enemy[j].y - 27 && b.y <= enemy[j].y) {
						enemy[j].hp--;
						b.alive = false;
						if (enemy[j].hp <= 0) {
							enemy[j].alive = false;
							aliveEnemy--;
						}
					}
				}
				break;
			}
			case eType::elf:
			{
				if (enemy->alive && b.x >= enemy->x - 15 && b.x <= enemy->x + 15 &&
					b.y >= enemy->y - 15 && b.y <= enemy->y + 15) {
					enemy->hp--;
					b.alive = false;
					if (enemy->hp <= 0) {
						enemy->alive = false;
						aliveEnemy--;
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

void Enemy::InitRound() {
	enemyIdx = 0;
	aliveEnemy = 0;	
	clear = true;
}





