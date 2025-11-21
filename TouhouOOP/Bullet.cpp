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

void Bullet::draw() {
	putimagePNG(x, y, bulletWidth, bulletHeight, &bullet1, 34, 133);
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
			Bullet Bullet(hero->x + 16, hero->y);
			Bullet.alive = true;
			bulletList.push_back(Bullet);
			next_fire = now + fire_cd;
		}
		fire = false;
		for (auto& b : bulletList) {
			if (b.isAlive()) {
				b.draw();
				b.move();
			}
		}
		bulletList.erase(
			std::remove_if(bulletList.begin(), bulletList.end(),
				[](const Bullet& b) { return !b.isAlive(); }),
			bulletList.end());
		//printf("Bullet List Size: %d\n", bulletList.size());
		break;

	default:
		break;
	}

}

