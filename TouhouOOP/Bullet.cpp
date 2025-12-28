#include "Bullet.h"

Bullet::Bullet(float _x, float _y) : Role(_x, _y) {
	loadimage(&bullet1, L"resource/bullet/bullet1.png");
	t1 = 0;
	t2 = 0;
	bulletWidth = 12;
	bulletHeight = 14;
	speed = 6.5f;
	alive = false;
	fire = false;	
}

Bullet::~Bullet() {
	for (auto* b : bulletList) {
		delete b;
	}
	bulletList.clear();
}

void Bullet::draw() {
	putimagePNG((int)x, (int)y, bulletWidth, bulletHeight, &bullet1, 34, 133);
}

void Bullet::move() {
	if (alive) {
		y -= speed;
		if (y <= TopEdge) alive = false;
	}
}

void Bullet::createBullet(Hero* hero, int type) {
	auto now = clock::now();
	switch (type)
	{
	case 1:
		if (now >= next_fire && fire) {
			Bullet* newBullet = new Bullet(hero->x + 16, hero->y);
			newBullet->alive = true;
			bulletList.push_back(newBullet); 
			next_fire = now + fire_cd;
		}
		fire = false;

		for (auto it = bulletList.begin(); it != bulletList.end(); ) {
			Bullet* b = *it;
			if (b->isAlive()) {
				b->move();
				b->draw();
			}
			if (!b->isAlive()) {
				delete b;
				it = bulletList.erase(it);
			}
			else {
				++it;
			}
		}
		//printf("Bullet List Size: %d\n", bulletList.size());
		break;

	default:
		break;
	}

}

