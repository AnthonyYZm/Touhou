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
	std::queue<SpawnEvent> empty;
	std::swap(eventQueue, empty);
	activeEvents.clear();

	for (const auto& ev : event) {
		eventQueue.push(ev);
	}

	waveStartTime = GetTickCount();
	clear = false;
}

void EnemyManager::updateSpawns() {
	// 如果全都空了，就没必要跑逻辑了
	if (clear && eventQueue.empty() && activeEvents.empty() && enemyList.empty()) return;

	DWORD now = GetTickCount();
	DWORD waveTime = now - waveStartTime;

	// 将所有时间到了的事件从队列移动到活跃列表
	while (!eventQueue.empty()) {
		// 获取队头事件
		SpawnEvent& ev = eventQueue.front();
		// 如果波次时间已经超过了该事件的 startTime，说明该开始生成这一组了
		if (waveTime >= (DWORD)ev.startTime) {
			// 初始化运行时状态
			ev.spawnedCount = 0;
			ev.lastSpawnTime = 0; 
			// 移动到活跃列表
			activeEvents.push_back(ev);
			eventQueue.pop();
		}
		else {
			break;
		}
	}

	// 遍历活跃列表，执行生成逻辑
	for (auto it = activeEvents.begin(); it != activeEvents.end(); ) {
		SpawnEvent& ev = *it; 
		if (ev.lastSpawnTime == 0 || (now - ev.lastSpawnTime >= (DWORD)ev.interval)) {

			createEnemy(ev); // 生成一个敌人

			ev.lastSpawnTime = now;
			ev.spawnedCount++;

			// 检查这一组是否生成完毕
			if (ev.spawnedCount >= ev.count) {
				// 生成完了，从活跃列表中移除
				it = activeEvents.erase(it);
				continue; 
			}
		}
		++it; 
	}
}

bool EnemyManager::checkEnemyClear() {
	if (enemyList.empty() && eventQueue.empty() && activeEvents.empty()) {
        return true;
    }
    else {
        return false;
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
	Enemy* newEnemy = new Enemy(ev.startX, ev.startY, ev.hp); 
	newEnemy->type = ev.type;
	newEnemy->alive = true;
	if (newEnemy->type == eType::boss) {
		newEnemy->maxHp = (ev.hp > 0) ? ev.hp : 2000; // 如果事件没设血量，给默认2000
		newEnemy->hp = newEnemy->maxHp;
		newEnemy->phase = 1; // 初始阶段
	}
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
		if (e == nullptr) continue;
		if (e->isAlive()) {
			e->move(); 
		}
	}
}

void EnemyManager::outBound() {
	for (auto it = enemyList.begin(); it != enemyList.end(); ) {
		Enemy* e = *it;
		bool isOut = (e->y > HEIGHT + 100 || e->x < -150 || e->x > WIDTH + 150 || e->y < -300);
		if (e->type != eType::boss && isOut) {
			delete e;
			it = enemyList.erase(it);
			continue;
		}
		++it;
	}
}

void EnemyManager::drawAll() {
	for (auto it = enemyList.begin(); it != enemyList.end(); ) {
		Enemy* e = *it;
		if (!e->isAlive()) {
			++it;
			continue;
		}
		if (e->type == eType::normal) {
			e->sx = e->row * 32.5f;
			e->sy = 322;
			e->frame = 4;
		}
		else if (e->type == eType::elf) {
			e->sx = e->row * 64;
			e->sy = 454;
			e->frame = 5;
		}
		e->texWidth = getEnemyWidth(*e);
		e->texHeight = getEnemyHeight(*e);
		e->width = (int)(e->texWidth * 1.5);
		e->height = (int)(e->texHeight * 1.5);
		e->draw();
		++it;	
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