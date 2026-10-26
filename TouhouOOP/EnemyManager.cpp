#pragma execution_character_set("utf-8")
#include "EnemyManager.h"
#include <unordered_set>

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
			if (ev.forceExitExisting) {
				beginExitForExistingEnemies();
				eventQueue.pop();
				continue;
			}
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

			createEnemy(ev); 

			ev.lastSpawnTime = now;
			ev.spawnedCount++;

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

void EnemyManager::beginExitForExistingEnemies() {
	const DWORD now = GetTickCount();
	for (Enemy* enemy : enemyList) {
		if (enemy == nullptr || !enemy->isAlive() || enemy->type == eType::boss) continue;
		const float startX = enemy->x;
		const float startY = enemy->y;
		const int exitStartMs = (int)(now - enemy->birthTime);
		enemy->setStrategy([startX, startY, exitStartMs](Enemy* target, int elapsedMs) {
			const int relativeMs = elapsedMs > exitStartMs ? elapsedMs - exitStartMs : 0;
			const float elapsedSeconds = relativeMs * 0.001f;
			target->x = startX;
			target->y = startY - 360.0f * elapsedSeconds;
			target->motionState = MotionState::EXITING;
			target->motionSegment = 0;
			target->fire = false;
		});
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
		newEnemy->maxHp = (ev.hp > 0) ? ev.hp : 2000; // 默认2000血
		newEnemy->hp = newEnemy->maxHp;
		newEnemy->phase = 1; 
	}
	// 注入移动策略
	newEnemy->setStrategy(ev.moveLogic);
	// 注入弹幕任务
	for (auto task : ev.initTasks) {
		// 普通敌人同样支持 startDelay，避免编队的首发在出生瞬间完全重叠。
		task.lastTime = newEnemy->birthTime - task.interval + task.startDelay;
		task.lastBurstTime = 0;
		task.currentBurst = 0;
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
		// 敌人的坐标使用游戏区域坐标，而不是窗口原点坐标。
		bool isOut = (e->y > Bottom + 100 || e->x < LeftEdge - 150 ||
			e->x > LeftEdge + WIDTH + 150 || e->y < TopEdge - 300);
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
			e->sx = (int)(e->row * 32.5f);
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
	// 统一回收当前场景中的敌人，避免遗留悬空指针。
	std::unordered_set<Enemy*> released;
	for (auto* e : enemyList) {
		if (e != nullptr && released.insert(e).second) {
			delete e;
		}
	}
	enemyList.clear();

	// 回收尚未刷出的 Boss 实例（挂在事件中）。
	auto releaseSpawnBoss = [&](SpawnEvent& ev) {
		if (ev.bossInstance != nullptr && released.insert(ev.bossInstance).second) {
			delete ev.bossInstance;
		}
		ev.bossInstance = nullptr;
	};

	while (!eventQueue.empty()) {
		SpawnEvent ev = eventQueue.front();
		eventQueue.pop();
		releaseSpawnBoss(ev);
	}
	for (auto& ev : activeEvents) {
		releaseSpawnBoss(ev);
	}
	activeEvents.clear();
}

void EnemyManager::InitRound() {
	enemyIdx = 0;
	aliveEnemy = 0;
	clear = true;
	clearEnemy();
}
