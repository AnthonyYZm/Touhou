#include "Item.h"
#include "Hero.h" 

IMAGE Item::imgItem;
bool Item::resLoaded = false;

Item::Item(float _x, float _y) : Role(_x, _y) {
	if (!resLoaded) {
		// 建议图片大小 16x16
		loadimage(&imgItem, L"resource/other/item.png");
		resLoaded = true;
	}
	vx = 0;
	vy = -3.0f; // 初始向上弹起
	magnet = false;
	alive = true;
}

Item::~Item() {}

void Item::draw() {
	putimagePNG((int)x, (int)y, 13, 13, &imgItem, 2, 2);
}

void Item::move() {
	if (!alive) return;

	if (magnet) {
		// 简单的追踪逻辑将在 Game 层处理，或者在这里传入目标
		// 为简化，这里只处理普通掉落
	}
	else {
		// 模拟抛物线：先向上减速，再向下加速
		vy += 0.1f; // 重力
		if (vy > 3.0f) vy = 3.0f; // 最大下落速度
		y += vy;
	}

	if (y > HEIGHT) alive = false;
}