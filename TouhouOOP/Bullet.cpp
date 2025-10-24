#include "Bullet.h"

Bullet::Bullet(float _x, float _y) : Role(_x, _y) {
	bulletWidth = 6;
	bulletHeight = 12;
	speed = 6;
	Alive = false;
}

void Bullet::draw() {
	setfillcolor(GREEN);
	fillcircle(x, y, 5);
}

void Bullet::move() {
	if (alive) {
		y -= speed;
		if (y <= TopEdge) Alive = false;
	}
}

