#pragma execution_character_set("utf-8")
#include "Item.h"
#include "Hero.h" 

IMAGE Item::imgItem;
bool Item::resLoaded = false;

Item::Item(float _x, float _y) : Role(_x, _y) {
	if (!resLoaded) {
		loadimage(&imgItem, L"resource/other/item.png");
		resLoaded = true;
	}
	vx = 0;
	vy = -3.0f;
	magnet = false;
	alive = true;
}

Item::~Item() {}

void Item::draw() {
	putimagePNG((int)x, (int)y, 13, 13, &imgItem, 2, 2, 18, 18);
}

void Item::move() {
	if (!alive) return;

	if (magnet) {
	}
	else {
		vy += 0.1f; 
		if (vy > 3.0f) vy = 3.0f; 
		y += vy;
	}

	if (y > HEIGHT) alive = false;
}