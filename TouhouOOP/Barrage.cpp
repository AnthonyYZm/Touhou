#include "Barrage.h"

Barrage::Barrage(float _x, float _y) : Role(_x, _y) {
	t1 = 0;
	t2 = 0;
	speed = 4.0f;
	alive = false;
	aliveBarr1 = 0;	
}

void Barrage::draw() {
	setfillcolor(RGB(255, 165, 0));
	fillcircle(x, y, 8);
}

void Barrage::move() {
	if (!alive) return;
	y += speed;
	if (y > HEIGHT) alive = false;
}