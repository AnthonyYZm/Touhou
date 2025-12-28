#include "Barrage.h"

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	loadimage(&barr1, L"resource/barrage/mid_bullet_darkGreen.png");
	t1 = 0;
	t2 = 0;
	speed = 4.0f;
	alive = false;
}

Barrage::~Barrage() {
	for (auto* b : barr1List) {
		delete b;
	}
	barr1List.clear();
	for (auto* b : barr2List) {
		delete b;
	}
	barr2List.clear();
}

void Barrage::draw() {
	putimagePNG((int)x, (int)y, 24, 24, &barr1, 0, 0);
}

void Barrage::move() {
	if (!alive) return;
	y += speed;
	if (y > HEIGHT) alive = false;
}


void Barrage::Normal(std::vector<Enemy*> enemy, Enemy* E) {
	t2 = GetTickCount();
	if (E->getEnemyNum() == E->GetAliveEnemy() && t2 - t1 >= 1000) {
		int cnt = E->GetAliveEnemy();
		for (int i = 0; i < cnt; i++) {
			if (!enemy[i]->isAlive()) continue;
			Barrage* newBarrage = new Barrage(enemy[i]->x, enemy[i]->y + 20);
			newBarrage->alive = true;
			barr1List.push_back(newBarrage);
		}
		t1 = t2;
	}
	for (auto it = barr1List.begin(); it != barr1List.end(); ) {
		Barrage* b = *it;
		if (b->isAlive()) {
			b->draw();
			b->move();
			if (b->x < 0 || b->x > WIDTH || b->y < 0 || b->y > HEIGHT) {
				b->alive = false;
			}
		}
		if (!b->isAlive()) {
			delete b;
			it = barr1List.erase(it);
		}
		else {
			++it;
		}
	}

	//printf("Barrage List Size: %d\n", barr1List.size());
}
//straight

void Barrage::Straight(Enemy* enemy, int gap, float speed, float &angle) {
	t2 = GetTickCount();
	if (enemy->alive && enemy->y >= CentralY && t2 - t1 >= gap) {
		Barrage* newBarrage = new Barrage(enemy->x + 32, enemy->y + 25);
		newBarrage->speed = speed;
		newBarrage->radian = angle * PI / 180.0f;
		newBarrage->alive = true;
		barr2List.push_back(newBarrage);
		angle += 8.0f;
		t1 = t2;
	}
	for (auto it = barr2List.begin(); it != barr2List.end(); ) {
		Barrage* b = *it;
		if (b->isAlive()) {
			b->draw();
			b->x += b->speed * cos(b->radian);
			b->y += b->speed * sin(b->radian);
			if (b->x < 0 || b->x > WIDTH || b->y < 0 || b->y > HEIGHT) {
				b->alive = false;
			}
		}
		if (!b->isAlive()) {
			delete b;
			it = barr2List.erase(it);
		}
		else {
			++it;
		}
	}
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

