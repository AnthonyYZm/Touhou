#pragma execution_character_set("utf-8")
#include "RLEnvironment.h"
#include "Library.h"
#include <algorithm>
#include <climits>
#include <cmath>

RLEnvironment::RLEnvironment(Hero* h, EnemyManager* e, std::vector<Barrage*>* b)
	: heroRef(h), enemiesRef(e), barrListRef(b), framesSurvived(0), currentLevel(1) {
}

void RLEnvironment::ClearBullets() {
	if (!barrListRef) return;
	// 训练回合重置时必须释放旧弹幕，否则长时间训练会不断堆积内存。
	for (auto* bullet : *barrListRef) {
		delete bullet;
	}
	barrListRef->clear();
}

Enemy* RLEnvironment::GetTrainingBoss() {
	auto& enemies = enemiesRef->getList();
	for (auto* enemy : enemies) {
		if (enemy && enemy->isAlive()) {
			return enemy;
		}
	}

	// 无菌室训练只需要一个稳定的参考点，不走完整 Boss 阶段逻辑，降低训练闭环复杂度。
	Enemy* dummy = new Enemy((float)CentralX, (float)(TopEdge + 120), INT_MAX);
	dummy->type = eType::boss;
	dummy->alive = true;
	dummy->fire = true;
	dummy->hp = INT_MAX;
	enemiesRef->addEnemy(dummy);
	return dummy;
}

void RLEnvironment::SpawnTrainingBullet(float x, float y, float vx, float vy) {
	// 训练弹幕直接写入 Barr.barrList，复用已有 Barrage 移动和尺寸信息。
	Barrage* bullet = new Barrage(x, y, BulletStyle::BLUE_);
	bullet->setVx(vx);
	bullet->setVy(vy);
	bullet->alive = true;
	barrListRef->push_back(bullet);
}

bool RLEnvironment::CheckHeroHit(const Barrage& bullet) const {
	const float dx = heroRef->x - bullet.x;
	const float dy = heroRef->y - bullet.y;
	const float heroRadius = (float)heroRef->JudgeR;
	const float bulletRadius = (float)(std::max)(bullet.getWidth(), bullet.getHeight()) * 0.35f;
	const float hitRadius = heroRadius + bulletRadius;
	return dx * dx + dy * dy <= hitRadius * hitRadius;
}

void RLEnvironment::UpdateTrainingBullets() {
	for (auto it = barrListRef->begin(); it != barrListRef->end(); ) {
		Barrage* bullet = *it;
		if (!bullet) {
			it = barrListRef->erase(it);
			continue;
		}

		bullet->move();

		// 这里使用游戏实际边界外的一圈缓冲，避免弹幕刚贴边就被提前删除。
		const bool outOfBounds =
			bullet->x < LeftEdge - 60.0f ||
			bullet->x > Right + 60.0f ||
			bullet->y < TopEdge - 60.0f ||
			bullet->y > Bottom + 60.0f;

		if (outOfBounds) {
			bullet->alive = false;
		}

		if (bullet->isAlive() && CheckHeroHit(*bullet)) {
			// 训练环境里一次中弹就结束本回合，让 reward 信号更清晰。
			heroRef->alive = false;
			bullet->alive = false;
		}

		if (!bullet->isAlive()) {
			delete bullet;
			it = barrListRef->erase(it);
		}
		else {
			++it;
		}
	}
}

void RLEnvironment::SortBulletsByDistance(std::vector<Barrage*>& sortedList) {
	std::sort(sortedList.begin(), sortedList.end(), [this](Barrage* a, Barrage* b) {
		const float distA = std::pow(a->x - heroRef->x, 2) + std::pow(a->y - heroRef->y, 2);
		const float distB = std::pow(b->x - heroRef->x, 2) + std::pow(b->y - heroRef->y, 2);
		return distA < distB;
		});
}

void RLEnvironment::Reset(int difficultyLevel) {
	currentLevel = difficultyLevel;
	framesSurvived = 0;

	// reset 只重置训练所需的最小状态，不启动普通关卡队列和完整演出系统。
	heroRef->alive = true;
	heroRef->fire = true;
	heroRef->x = (float)CentralX;
	heroRef->y = (float)(Bottom - 100);
	heroRef->invincible = false;

	ClearBullets();
	enemiesRef->InitRound();

	GetTrainingBoss();
}

void RLEnvironment::Step(int action) {
	const float speed = 4.0f;
	float dx = 0.0f;
	float dy = 0.0f;

	switch (action) {
	case 1: dy = -speed; break;
	case 2: dy = speed; break;
	case 3: dx = -speed; break;
	case 4: dx = speed; break;
	default: break;
	}

	heroRef->x += dx;
	heroRef->y += dy;

	if (heroRef->x < LeftEdge) heroRef->x = (float)LeftEdge;
	if (heroRef->x > Right) heroRef->x = (float)Right;
	if (heroRef->y < TopEdge) heroRef->y = (float)TopEdge;
	if (heroRef->y > Bottom) heroRef->y = (float)Bottom;

	framesSurvived++;

	Enemy* boss = GetTrainingBoss();
	if (currentLevel >= 1) {
		// Level 1 使用固定车道弹幕 + 少量自机狙，先训练基础移动和避弹反应。
		if (framesSurvived % 18 == 0) {
			const int lane = (framesSurvived / 18) % 6;
			const float x = LeftEdge + 70.0f + lane * 115.0f;
			SpawnTrainingBullet(x, (float)(TopEdge + 70), 0.0f, 3.4f);
		}

		if (framesSurvived % 45 == 0) {
			const float fromX = boss ? boss->x : (float)CentralX;
			const float fromY = boss ? boss->y : (float)(TopEdge + 120);
			float aimX = heroRef->x - fromX;
			float aimY = heroRef->y - fromY;
			float len = std::sqrt(aimX * aimX + aimY * aimY);
			if (len < 0.01f) len = 1.0f;
			const float bulletSpeed = 3.0f;
			SpawnTrainingBullet(fromX, fromY, aimX / len * bulletSpeed, aimY / len * bulletSpeed);
		}
	}

	UpdateTrainingBullets();
}

RLState RLEnvironment::GetObservation() {
	RLState state;
	state.clear();

	// 自机位置使用 0~1 归一化，子弹和 Boss 使用相对位移，方便模型泛化。
	state.heroX = (heroRef->x - LeftEdge) / (float)WIDTH;
	state.heroY = (heroRef->y - TopEdge) / (float)HEIGHT;

	Enemy* boss = GetTrainingBoss();
	if (boss) {
		state.bossRelativeX = (boss->x - heroRef->x) / (float)WIDTH;
		state.bossRelativeY = (boss->y - heroRef->y) / (float)HEIGHT;
	}

	std::vector<Barrage*> sortedBullets;
	for (auto* bullet : *barrListRef) {
		if (bullet && bullet->isAlive()) {
			sortedBullets.push_back(bullet);
		}
	}
	SortBulletsByDistance(sortedBullets);

	// 只暴露最近的固定数量弹幕，保持神经网络输入维度稳定。
	const int count = (std::min)(MAX_OBSERVE_BULLETS, (int)sortedBullets.size());
	for (int i = 0; i < count; ++i) {
		state.bulletRelativeX[i] = (sortedBullets[i]->x - heroRef->x) / (float)WIDTH;
		state.bulletRelativeY[i] = (sortedBullets[i]->y - heroRef->y) / (float)HEIGHT;
	}

	return state;
}

float RLEnvironment::CalculateReward() {
	float reward = 0.1f;

	// 基础奖励鼓励存活；死亡大惩罚让模型优先学会避弹。
	if (!heroRef->isAlive()) {
		reward -= 100.0f;
	}

	const bool nearWall =
		heroRef->x <= LeftEdge + 10.0f ||
		heroRef->x >= Right - 10.0f ||
		heroRef->y <= TopEdge + 10.0f ||
		heroRef->y >= Bottom - 10.0f;
	if (nearWall) {
		reward -= 0.5f;
	}

	return reward;
}

bool RLEnvironment::IsDone() {
	return (!heroRef->isAlive()) || (framesSurvived > 3000);
}
