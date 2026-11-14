#pragma execution_character_set("utf-8")
#include "EnemyManager.h"
#include <utility>

EnemyManager::EnemyManager() {
	aliveEnemy = 0;
}

EnemyManager::~EnemyManager() {
	for (auto* e : enemyList) delete e;
	enemyList.clear();
}


void EnemyManager::scheduleSpawnGroup(
	const SpawnGroupDef& group,
	std::uint64_t worldFrame) {
	if (group.count <= 0) return; // 数量无效时不排入生成计划

	PendingSpawnGroup pending;
	pending.definition = group;        // 保存这组敌人的静态参数
	pending.nextSpawnFrame = worldFrame; // 第一只在当前世界帧生成

	// 把毫秒间隔转成逻辑帧。
	// 间隔为 0 时仍至少隔 1 帧生成下一只。
	pending.intervalFrames = group.intervalMs <= 0
		? 1
		: GameFramesFromMilliseconds(
			static_cast<std::uint64_t>(group.intervalMs));

	pendingSpawnGroups.push_back(std::move(pending));
}

void EnemyManager::updateSpawns(std::uint64_t worldFrame) {
	for (auto it = pendingSpawnGroups.begin();
		it != pendingSpawnGroups.end();) {
		if (worldFrame < it->nextSpawnFrame) {
			++it; // 这组还没到下一只敌人的生成时间
			continue;
		}

		createEnemy(it->definition); // 生成一只
		++it->spawnedCount;

		if (it->spawnedCount >= it->definition.count) {
			// 整组生成完毕，移除运行时计划。
			it = pendingSpawnGroups.erase(it);
		}
		else {
			// 计算这组下一只敌人的目标生成帧。
			it->nextSpawnFrame += it->intervalFrames;
			++it;
		}
	}
}


// 清场判定同时检查已生成敌人和仍在生成中的组。
// 否则可能在一组尚未刷完时错误地认为场上已经清空。
bool EnemyManager::checkEnemyClear() {
	return enemyList.empty() && pendingSpawnGroups.empty();
}

void EnemyManager::retreatNonBossEnemies() {
	const DWORD now = GetTickCount(); // 当前系统毫秒时钟，用于对齐敌人出生计时
	for (Enemy* enemy : enemyList) {
		if (enemy == nullptr || !enemy->isAlive() || enemy->type == eType::boss) continue;
		const float startX = enemy->x; // 退场开始时固定的水平位置
		const float startY = enemy->y; // 退场开始时的位置
		const int exitStartMs = (int)(now - enemy->birthTime); // 退场开始时的敌人年龄
		enemy->setStrategy([startX, startY, exitStartMs](Enemy* target, int elapsedMs) {
			const int relativeMs = elapsedMs > exitStartMs ? elapsedMs - exitStartMs : 0; // 进入退场后的时间
			const float elapsedSeconds = relativeMs * 0.001f; // 转为秒以计算离场距离
			target->x = startX;
			target->y = startY - 360.0f * elapsedSeconds;
			target->motionState = MotionState::EXITING;
			target->motionSegment = 0;
			target->fire = false;
		});
	}
}

void EnemyManager::createEnemy(const SpawnGroupDef& group) {
	Enemy* newEnemy = new Enemy(group.startX, group.startY, group.hp);
	newEnemy->type = group.type;
	newEnemy->alive = true;
	if (newEnemy->type == eType::boss) {
		newEnemy->maxHp = (group.hp > 0) ? group.hp : 2000; // 默认2000血
		newEnemy->hp = newEnemy->maxHp;
		newEnemy->phase = 1; 
	}
	// 注入移动策略
	newEnemy->setStrategy(group.moveLogic);
	// 注入弹幕任务
	for (auto task : group.initTasks) {
		// 普通敌人同样支持 startDelay，避免编队的首发在出生瞬间完全重叠。
		task.lastTime = newEnemy->birthTime - task.interval + task.startDelay;
		task.lastBurstTime = 0;
		task.currentBurst = 0;
		newEnemy->AddTask(task);
	}
	enemyList.push_back(newEnemy);
	aliveEnemy++;
}

void EnemyManager::moveEnemy(std::uint64_t worldFrame) {
	updateSpawns(worldFrame);
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
	for (auto* e : enemyList) {
		delete e;
	}
	enemyList.clear();
	pendingSpawnGroups.clear();
}

void EnemyManager::InitRound() {
	aliveEnemy = 0;
	clearEnemy();
}
