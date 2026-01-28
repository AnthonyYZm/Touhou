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

void EnemyManager::setWave(const std::vector<SpawnEvent>& event) {
	// 清空旧队列
	std::queue<SpawnEvent> empty;
	std::swap(eventQueue, empty);

	// 填入新事件
	for (const auto& ev : event) {
		eventQueue.push(ev);
	}

	waveStartTime = GetTickCount();
	processingEvent = false;
	clear = false;
}

void EnemyManager::updateSpawns() {
	if (clear && eventQueue.empty() && enemyList.empty()) return;

	DWORD now = GetTickCount();
	DWORD waveTime = now - waveStartTime;

	// 1. 检查是否有新事件需要开始
	if (!processingEvent && !eventQueue.empty()) {
		if (waveTime >= (DWORD)eventQueue.front().startTime) {
			currentEvent = eventQueue.front();
			eventQueue.pop();
			processingEvent = true;
			spawnedCount = 0;
			lastSpawnTime = 0;
		}
	}

	// 2. 执行当前生成事件
	if (processingEvent) {
		if (now - lastSpawnTime >= (DWORD)currentEvent.interval) {
			createEnemy(currentEvent);
			lastSpawnTime = now;
			spawnedCount++;

			if (spawnedCount >= currentEvent.count) {
				processingEvent = false; 
			}
		}
	}
}

void EnemyManager::createEnemy(const SpawnEvent& ev) {
	// 特殊情况：如果是 Boss
	if (ev.bossInstance != nullptr) {
		ev.bossInstance->alive = true;
		enemyList.push_back(ev.bossInstance);
		aliveEnemy++;
		return;
	}
	// 普通敌人生成
	Enemy* newEnemy = new Enemy(ev.startX, ev.startY, ev.hp); // hp暂时写死或加入结构体
	newEnemy->type = ev.type;
	newEnemy->alive = true;
	// 注入移动策略
	newEnemy->setStrategy(ev.moveLogic);
	// 注入弹幕任务
	for (const auto& task : ev.initTasks) {
		newEnemy->AddTask(task);
	}
	enemyList.push_back(newEnemy);
	aliveEnemy++;
}

void EnemyManager::moveEnemy() {
	updateSpawns(); // 先看看有没有新怪要刷
	for (auto it = enemyList.begin(); it != enemyList.end(); ++it) {
		Enemy* e = *it;
		if (e->isAlive()) {
			e->move(); // 多态调用：Boss调Boss的，小兵调MoveStrategy
		}
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

bool EnemyManager::collision(Bullet* bullet) {
	bool hitAny = false;
	for (auto& b : bullet->bulletList) {
		if (!b->alive) continue;

		for (auto it = enemyList.begin(); it != enemyList.end(); ) {
			Enemy* enemy = *it;
			if (!enemy->isAlive()) { ++it; continue; }

			// 自动根据敌人当前类型选择碰撞框宽度/高度
			int w = (enemy->type == eType::elf) ? Enemy::getElfWidth() : Enemy::getNormalWidth();
			int h = (enemy->type == eType::elf) ? Enemy::getElfHeight() : Enemy::getNormalHeight();

			if (b->x + Bullet::getBulletWidth() >= enemy->x && b->x <= enemy->x + w &&
				b->y + Bullet::getBulletHeight() >= enemy->y && b->y <= enemy->y + h)
			{
				enemy->hp--;
				b->alive = false;
				if (enemy->hp <= 0) {
					enemy->alive = false;
					aliveEnemy--;
					delete enemy;
					it = enemyList.erase(it);
					hitAny = true;
					continue;
				}
			}
			++it;
		}
	}
	return hitAny;
}

bool EnemyManager::checkEnemyClear() {
	if (aliveEnemy == 0 && eventQueue.empty() && !processingEvent) {
		return true;
	}
	else {
		return false;
	}
}

void EnemyManager::clearEnemy() {
	for (auto* e : enemyList) {
		delete e;
	}
}

void EnemyManager::InitRound() {
	enemyIdx = 0;
	aliveEnemy = 0;
	clear = true;
	for (auto* e : enemyList) delete e;
	enemyList.clear();
}