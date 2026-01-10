#include "Barrage.h"

int const Barrage::darkGreenWidth = 24;
int const Barrage::darkGreenHeight = 24;

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	loadimage(&barr1, L"resource/barrage/mid_bullet_darkGreen.png");
	t1 = 0;
	t2 = 0;
	speed = 4.0f;
	alive = false;
	currentAngle = 90.0f;
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

void Barrage::reset() {
	t1 = GetTickCount();
	currentAngle = 90.0f;
}

void Barrage::draw() {
	putimagePNG((int)x, (int)y, darkGreenWidth, darkGreenHeight, &barr1, 0, 0);
}

void Barrage::move() {
	if (!alive) return;
	y += speed;
	if (y > HEIGHT) alive = false;
}


void Barrage::Normal(std::vector<Enemy*>& enemyList, EnemyManager* E) {
	t2 = GetTickCount();
	if (E->getAliveEnemy() && t2 - t1 >= 1000) {
		for (auto* e : enemyList) {
			if (!e->isAlive() || !e->fire) continue;
			Barrage* newBarrage = new Barrage(e->x + Enemy::getNormalWidth() / 2 - darkGreenWidth / 2, e->y + Enemy::getNormalHeight() / 2 - darkGreenHeight / 2);
			newBarrage->alive = true;
			barr1List.push_back(newBarrage);
		}
		t1 = t2;
	}

}
//straight

void Barrage::straightMill(std::vector<Enemy*>& enemyList, int gap, float speed, float &angle) {
	t2 = GetTickCount();
	for (auto* e : enemyList) {
		if (e->alive && e->fire && t2 - t1 >= gap) {
			Barrage* newBarrage = new Barrage(e->x + Enemy::getElfWidth() / 2 - darkGreenWidth / 2, e->y + Enemy::getElfHeight() / 2 - darkGreenHeight / 2);
			newBarrage->speed = speed;
			newBarrage->radian = currentAngle * PI / 180.0f;
			newBarrage->alive = true;
			barr2List.push_back(newBarrage);
			currentAngle += 8.0f;
			t1 = t2;
		}
	}
}

void Barrage::update() {

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

