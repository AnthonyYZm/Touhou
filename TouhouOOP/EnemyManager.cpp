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
			newEnemy->type = eType::normal;
			enemyList.push_back(newEnemy);
			enemyIdx++;
			aliveEnemy++;
			clear = false;
			t1 = t2;
		}
		break;
	case eType::elf:
		// Boss 1
		if (clear) {
			Enemy* elfEnemy = new Enemy();
			elfEnemy->x = WIDTH * 0.5f + LeftEdge;
			elfEnemy->y = TopEdge;
			elfEnemy->alive = true;
			elfEnemy->hp = 10;
			elfEnemy->type = eType::elf;
			elfEnemy->frame = 5;
			enemyList.push_back(elfEnemy);
			aliveEnemy = 1;
			clear = false;
		}
		break;
	default:
		break;
	}
}

void EnemyManager::drawAll() {
	for (auto it = enemyList.begin(); it != enemyList.end(); ) {
		Enemy* e = *it;
		if (!e->isAlive()) {
			++it;
			continue;
		}
		switch (e->type) {
			case eType::normal:
				e->sx = e->row * 32.5f;
				e->sy = 322;
				e->frame = 4;
				e->width = Enemy::getNormalWidth();
				e->height = Enemy::getNormalHeight();
				break;
			case eType::elf:
				e->sx = e->row * 64;
				e->sy = 454;
				e->frame = 5;
				e->width = Enemy::getElfWidth();
				e->height = Enemy::getElfHeight();
				break;
			default:
				break;
			}
		e->draw();
		++it;	
	}
}

 void EnemyManager::moveEnemy() {
	for (auto it = enemyList.begin(); it != enemyList.end(); ++it) {
		Enemy* e = *it;
		if (!e->isAlive()) continue;
		eType type = e->type;
		float speed = Enemy::speedMap[type];
		switch (type)
		{
		case eType::normal:
		{
			e->fire = true;
			e->setVy(speed);
			if (e->x < WIDTH / 2 + LeftEdge) {
				e->setVx(speed * 0.5f);
			}
			else {
				e->setVx(-speed * 0.5f);
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
		}
		break;
		case eType::elf:
			e->setVx(0);
			if (e->y <= CentralY) {
				e->setVy(speed);
			}
			else {
				e->setVy(0);
				e->fire = true;
			}
			break;
		default:
			break;
		}
		e->move();
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