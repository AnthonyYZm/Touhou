#include "Barrage.h"

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	loadimage(&barr1, L"resource/barrage/mid_bullet_darkGreen.png");
	t1 = 0;
	t2 = 0;
	speed = 4.0f;
	alive = false;
}

void Barrage::draw() {
	putimagePNG(x, y, 24, 24, &barr1, 0, 0);
}

void Barrage::move() {
	if (!alive) return;
	y += speed;
	if (y > HEIGHT) alive = false;
}


void Barrage::Normal(Enemy* enemy, Enemy* E) {
	t2 = GetTickCount();
	if (E->getEnemyNum() == E->GetAliveEnemy() && t2 - t1 >= 1000) {
		int cnt = E->GetAliveEnemy();
		for (int i = 0; i < cnt; i++) {
			if (!enemy[i].isAlive()) continue;
			Barrage b(enemy[i].x, enemy[i].y + 20);
			b.alive = true;
			barr1List.push_back(b);
		}
		t1 = t2;
	}
	for (auto& b : barr1List) {
		if (b.isAlive()) {
			b.draw();
			b.move();
			if (b.x < 0 || b.x > WIDTH || b.y < 0 || b.y > HEIGHT) {
				b.alive = false;
			}
		}
	}
	barr1List.erase(
		std::remove_if(barr1List.begin(), barr1List.end(),
			[](const Barrage& b) { return !b.isAlive(); }),
		barr1List.end());
	//printf("Barrage List Size: %d\n", barr1List.size());
}
//straight

void Barrage::Straight(Enemy* enemy, int gap, float speed, float &angle) {
	t2 = GetTickCount();
	if (enemy->alive && enemy->y >= CentralY && t2 - t1 >= gap) {
		Barrage b(enemy->x + 32, enemy->y + 25);
		b.speed = speed;
		b.radian = angle * PI / 180.0f;
		b.alive = true;
		barr2List.push_back(b);
		angle += 8.0f;
		t1 = t2;
	}
	for (auto& b : barr2List) {
		if (b.isAlive()) {
			b.draw();
			b.x += b.speed * cos(b.radian);
			b.y += b.speed * sin(b.radian);
			if (b.x < 0 || b.x > WIDTH || b.y < 0 || b.y > HEIGHT) {
				b.alive = false;
			}
		}
	}
	barr2List.erase(
		std::remove_if(barr2List.begin(), barr2List.end(),
			[](const Barrage& b) { return !b.isAlive(); }),
		barr2List.end());
}

//windmill
void Barrage::Windmill(Enemy* enemy, int bladeNum) {
	float R = 300.0f;
	t2 = GetTickCount();
	if (enemy->alive && enemy->y > HEIGHT / 3 && t2 - t1 >= 600) {
		for (int i = 0; i < bladeNum; i++) {
		}
	}
}

