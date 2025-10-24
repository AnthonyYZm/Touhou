#include "EnemyRule.h"	

EnemyRule::EnemyRule() {
	aliveEnemy = 0;
	t1 = GetTickCount();
	t2 = 0;
}

void EnemyRule::EnemyX() { 
	enemy[0].EnemyX(); 
}

float EnemyRule::getEnemyX() { 
	return enemy[0].getEnemyX(); 
}

void EnemyRule::EnemyNum() { 
	enemy[0].EnemyNum(); 
}

int EnemyRule::getEnemyNum() { 
	return enemy[0].getEnemyNum(); 
}

void EnemyRule::Init() {
	for (int i = 0; i < aliveEnemy; i++) {
		enemy[i].Init();
	}	
	clear = false;
}

void EnemyRule::createEnemy() {	
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

void EnemyRule::move(float speed) {
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

bool EnemyRule::checkEnemyClear() {
	for (int i = 0; i < aliveEnemy; i++) {
		if (enemy[i].isAlive()) {
			clear = false;
			return false;
		}
	}	
	clear = true;
	return true;
}

void EnemyRule::collision(std::vector<Bullet>& bullets) {
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

void EnemyRule::InitRound() {
	aliveEnemy = 0;
	clear = true;
}