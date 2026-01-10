#include "EnemyManager.h"



EnemyManager::EnemyManager() {
	aliveEnemy = 0; 
	enemyIdx = 0;
	clear = true;
	t1 = 0; t2 = 0;
}

EnemyManager::~EnemyManager() {
	for (auto* e : enemyList) delete e;
	enemyList.clear();
}

void EnemyManager::SetStatus(const std::vector<float>& positions, int interval) {
	this->currentPosition = positions; // 复制坐标列表
	this->interval = interval;
	this->enemyIdx = 0; // 重置计数器
}

void EnemyManager::createEnemy(const eType& type) {
	t2 = GetTickCount();
	switch (type) {
	case eType::normal:
		// Creat normal enemy
		if (t2 - t1 >= 1000 && enemyIdx < currentPosition.size()) {
			float targetX = currentPosition[enemyIdx];
			Enemy* newEnemy = new Enemy();
			newEnemy->x = targetX;
			newEnemy->y = TopEdge;
			newEnemy->alive = true;
			newEnemy->hp = 1;
			enemyList.push_back(newEnemy);
			enemyIdx++;
			aliveEnemy++;
			clear = false;
			t1 = t2;
		break;
	}
	case eType::elf:
		// Boss 1
		if (clear) {
			Enemy* elfEnemy = new Enemy();
			elfEnemy->x = WIDTH * 0.5f + LeftEdge;
			elfEnemy->y = TopEdge;
			elfEnemy->alive = true;
			elfEnemy->hp = 10;
			enemyList.push_back(elfEnemy);
			aliveEnemy = 1;
			clear = false;
		}
		break;
	default:
		break;
	}
}

void EnemyManager::drawAll(const eType& type) {
	for (auto* e : enemyList) {
		if (!e->isAlive()) continue;
		switch (type) {
		case eType::normal:
			e->draw();
			break;
		case eType::elf:
			e->draw2();
			break;
		default:
			break;
		}
	}
}

void EnemyManager::move(const eType& type, float speed) {
	switch (type)
	{
	case eType::normal:
		for (auto* e : enemyList) {
			e->fire = true;
			if (e->x < WIDTH / 2 + LeftEdge) {
				e->y += speed;
				e->x += speed * 0.5f;
			}
			else {
				e->y += speed;
				e->x -= speed * 0.5f;
			}
		}
		/*
		for (size_t i = 0; i < enemyList.size(); i++) {
			float X = currentPosition[i];
			Enemy* cur = enemyList[i];
			if (cur->y <= HEIGHT / 3 + 16) {
				cur->y += speed;
				(getEnemyX() <= WIDTH / 2 + 32) ? cur->x += (speed * 0.5f) : cur->x -= (speed * 0.5f);
			}
			if (getEnemyX() <= WIDTH / 2 + 32 && cur->y > HEIGHT / 3 + 16) {
				if (enemyList[0]->x <= WIDTH / 3 * 2 + 32 && i == 0) {
					enemyList[0]->x += speed;
				}
				if (i > 0 && (cur->x <= enemyList[i - 1]->x - 50)) {
					cur->x += speed;
				}
			}
			else if (getEnemyX() > WIDTH / 2 + 32 && cur->y > HEIGHT / 3 + 16) {
				if (enemyList[0]->x >= WIDTH / 3 + 32 && i == 0) {
					enemyList[0]->x -= speed;
				}
				if (i > 0 && (cur->x >= enemyList[i - 1]->x + 50)) {
					cur->x -= speed;
				}
			}
		}*/
	break;

	case eType::elf:
		for (auto* elf : enemyList) {
			(elf->y <= CentralY) ? (elf->y += speed) : (elf->fire = true);
		}
		break;

	default:
		break;
	}
}

bool EnemyManager::collision(const eType& type, Bullet* bullet) {
	bool hitAny = false;
	for (auto& b : bullet->bulletList) {
		if (b->alive) {
			for (auto it = enemyList.begin(); it != enemyList.end();) {
				Enemy* enemy = *it;
				bool hit = false;

				switch (type)
				{
				case eType::normal:
					if (enemy->alive && b->x + Bullet::getBulletWidth() >= enemy->x && b->x <= enemy->x + Enemy::getNormalWidth() &&
						b->y + Bullet::getBulletHeight() >= enemy->y && b->y <= enemy->y + Enemy::getNormalHeight()) {
						hit = true;
					}
					break;

				case eType::elf:
					if (enemy->alive && b->x + Bullet::getBulletWidth() >= enemy->x && b->x <= enemy->x + Enemy::getElfWidth() &&
						b->y + Bullet::getBulletHeight() >= enemy->y && b->y <= enemy->y + Enemy::getElfHeight()) {
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
						it = enemyList.erase(it); // erase 返回下一个有效的迭代器
						continue; // 继续下一次循环，不执行 ++it
					}
				}
				++it;
			}
		}
	}
	return hitAny;
}

bool EnemyManager::checkEnemyClear() {
	if (aliveEnemy == 0) return true;
	else return false;
}

void EnemyManager::InitRound() {
	enemyIdx = 0;
	aliveEnemy = 0;
	clear = true;
	for (auto* e : enemyList) delete e;
	enemyList.clear();
}