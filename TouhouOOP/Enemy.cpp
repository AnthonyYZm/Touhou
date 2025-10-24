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
	aliveEnemy = 0;	
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

void Enemy::Init(Enemy* enemy) {
	for (int i = 0; i < aliveEnemy; i++) {
		enemy[i].Init();
	}
	clear = false;
}

void Enemy::createEnemy(Enemy* enemy) {
	t2 = GetTickCount();
	if (t2 - t1 >= 800 && aliveEnemy < getEnemyNum()) {
		enemy[aliveEnemy].x = getEnemyX();
		enemy[aliveEnemy].y = 0;
		enemy[aliveEnemy].alive = true;
		aliveEnemy++;
		t1 = t2;
	}
	for (int i = 0; i < aliveEnemy; i++) {
		if (enemy[i].isAlive()) enemy[i].draw();
	}
}

void Enemy::move(float speed, Enemy* enemy) {
	for (int i = 0; i < aliveEnemy; i++) {
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

bool Enemy::checkEnemyClear(Enemy* enemy) {
	for (int i = 0; i < aliveEnemy; i++) {
		if (enemy[i].isAlive()) {
			clear = false;
			return false;
		}
	}
	clear = true;
	return true;
}

void Enemy::collision(std::vector<Bullet>& bullets, Enemy* enemy) {
	for (auto& b : bullets) {
		if (b.Alive) {
			for (int j = 0; j < aliveEnemy; j++) {
				if (enemy[j].alive && b.x >= enemy[j].x && b.x <= enemy[j].x + 32 &&
					b.y >= enemy[j].y - 27 && b.y <= enemy[j].y) {
					enemy[j].hp--;
					b.Alive = false;
					if (enemy[j].hp <= 0) {
						enemy[j].alive = false;
					}
				}
			}
		}
	}
}

void Enemy::InitRound() {
	aliveEnemy = 0;
	clear = true;
}





