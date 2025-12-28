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
std::map<eType, float> Enemy::speedMap = {
	{eType::normal, 4.5f},
	{eType::elf, 4.5f}
};

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
	putimagePNG((int)x, (int)y, 32.5f, 27.0f, &enemy1, sx, sy);
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
	putimagePNG((int)x, (int)y, 64, 50, &enemy1, sx, sy);
	if (te2 - te1 > 80) {
		row = (row + 1) % frame;
		te1 = te2;
	}
}

void Enemy::createEnemy(const eType& type, std::vector<Enemy*>& list) {
	t2 = GetTickCount();
	switch (type) {
		case eType::normal:
			// Creat normal enemy
		{
			if (t2 - t1 >= 1000 && enemyIdx < getEnemyNum()) {
				Enemy* newEnemy = new Enemy();
				newEnemy->x = enemyX;
				newEnemy->y = TopEdge;
				newEnemy->alive = true;
				newEnemy->hp = 1;
				list.push_back(newEnemy);
				enemyIdx++;
				aliveEnemy++;	
				clear = false;	
				t1 = t2;
			}
			for (auto& e : list) {
				if (e->alive) e->draw();
			}
			break;
		}
		case eType::elf:
			// Boss 1
		{
			if (clear) {
				Enemy* elfEnemy = new Enemy();
				elfEnemy->x = WIDTH * 0.5f + LeftEdge;
				elfEnemy->y = TopEdge;
				elfEnemy->alive = true;
				elfEnemy->hp = 10;
				list.push_back(elfEnemy);
				aliveEnemy = 1;
				clear = false;
			}
			for (auto& elf : list) {
				elf->draw2();
			}
			break;
		}
		default:
			break;
	}
}

void Enemy::move(const eType& type, std::vector<Enemy*>& list, float speed) {
	switch (type)
	{
	case eType::normal:
	{
		for (size_t i = 0; i < list.size(); i++) {
			Enemy* cur = list[i];
			if (cur->y <= HEIGHT / 3 + 16) {
				cur->y += speed;
				(getEnemyX() <= WIDTH / 2 + 32) ? cur->x += (speed * 0.5f) : cur->x -= (speed * 0.5f);
			}
			if (getEnemyX() <= WIDTH / 2 + 32 && cur->y > HEIGHT / 3 + 16) {
				if (list[0]->x <= WIDTH / 3 * 2 + 32 && i == 0) {
					list[0]->x += speed;
				}
				if (i > 0 && (cur->x <= list[i - 1]->x - 50)) {
					cur->x += speed;
				}
			}
			else if (getEnemyX() > WIDTH / 2 + 32 && cur->y > HEIGHT / 3 + 16) {
				if (list[0]->x >= WIDTH / 3 + 32 && i == 0) {
					list[0]->x -= speed;
				}
				if (i > 0 && (cur->x >= list[i - 1]->x + 50)) {
					cur->x -= speed;
				}
			}
		}
	}
	break;
		
	case eType::elf:
		for (auto* elf : list) {
			if (elf->y <= CentralY) {
				elf->y += speed;
			}
		}
		break;

	default:
		break;
	}
}

bool Enemy::checkEnemyClear() {
	if (aliveEnemy == 0) return true;
	else return false;
}

bool Enemy::collision(const eType& type, Bullet* bullet, std::vector<Enemy*>& list) {
	bool hitAny = false;
	for (auto& b : bullet->bulletList) {
		if (b->alive) {
			for (auto it = list.begin(); it != list.end();) {
				Enemy* enemy = *it;
				bool hit = false;	

				switch (type)
				{
					case eType::normal:
						if (enemy->alive && b->x >= enemy->x && b->x <= enemy->x + 32 &&
							b->y >= enemy->y - 27 && b->y <= enemy->y) {
							hit = true;
						}
					break;

					case eType::elf:
						if (enemy->alive && b->x >= enemy->x && b->x <= enemy->x + 64 &&
							b->y >= enemy->y - 50 && b->y <= enemy->y) {
							hit = true;
						}
					break;

				default:
					break;
				}

				if (hit) {
					enemy->hp--;
					b->alive = false;
					if (enemy->hp <= 0) {
						enemy->alive = false;
						aliveEnemy--;
						hitAny = true;
						delete enemy;
						it = list.erase(it); // erase 返回下一个有效的迭代器
						continue; // 继续下一次循环，不执行 ++it
					}
				}
				++it;
			}
		}
	}
	return hitAny;
}

void Enemy::InitRound() {
	enemyIdx = 0;
	aliveEnemy = 0;	
	clear = true;
}





